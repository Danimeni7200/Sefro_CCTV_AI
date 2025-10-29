from fastapi import FastAPI, UploadFile, File, Form, HTTPException
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from datetime import datetime, timezone
from typing import Optional, List
import hashlib
import requests
import os
import json
import orjson
from pathlib import Path
from PIL import Image
import io
import numpy as np


LOG_DIR = Path(__file__).parent / "logs"
LOG_DIR.mkdir(parents=True, exist_ok=True)
LOG_FILE = LOG_DIR / "inference.jsonl"

# Config via environment
AI_CONF_THRESHOLD = float(os.getenv("CONFIDENCE_THRESHOLD", "0.7"))
CLOUD_API_URL = os.getenv("CLOUD_API_URL", "http://127.0.0.1:9000")
CLOUD_SYNC_ENABLED = os.getenv("CLOUD_SYNC_ENABLED", "false").lower() == "true"
CLOUD_API_TOKEN = os.getenv("CLOUD_API_TOKEN", "")


class BBox(BaseModel):
    x: float
    y: float
    w: float
    h: float


class LPRResult(BaseModel):
    timestamp: str
    camera_id: str
    plate_text: str
    confidence: float
    bbox: BBox
    vehicle_color: Optional[str] = None
    vehicle_type: Optional[str] = None
    edge_image_path: Optional[str] = None


app = FastAPI(title="Local LPR AI Service")

# Optional models (YOLOv8 + EasyOCR) with lazy init
YOLO_MODEL = None
EASYOCR_READER = None

def get_yolo_model():
    global YOLO_MODEL
    if YOLO_MODEL is not None:
        return YOLO_MODEL
    try:
        from ultralytics import YOLO
        # Pretrained generic model; replace with plate-specific weights when available
        YOLO_MODEL = YOLO("yolov8n.pt")
    except Exception:
        YOLO_MODEL = None
    return YOLO_MODEL

def get_easyocr():
    global EASYOCR_READER
    if EASYOCR_READER is not None:
        return EASYOCR_READER
    try:
        import easyocr
        EASYOCR_READER = easyocr.Reader(["en"])  # customize languages as needed
    except Exception:
        EASYOCR_READER = None
    return EASYOCR_READER


def write_log(record: dict) -> None:
    with LOG_FILE.open("a", encoding="utf-8") as f:
        f.write(orjson.dumps(record).decode("utf-8"))
        f.write("\n")


def dummy_detect_and_ocr(image_bytes: bytes) -> tuple[BBox, str, float]:
    # Placeholder: returns center bbox and mock plate text with medium confidence
    try:
        img = Image.open(io.BytesIO(image_bytes)).convert("RGB")
        w, h = img.size
        bw, bh = w * 0.4, h * 0.15
        bx = (w - bw) / 2
        by = (h - bh) / 2
        bbox = BBox(x=float(bx), y=float(by), w=float(bw), h=float(bh))
        plate_text = "ABC1234"
        confidence = 0.78
        return bbox, plate_text, confidence
    except Exception:
        # Fallback fixed bbox and low confidence
        bbox = BBox(x=0.0, y=0.0, w=0.0, h=0.0)
        return bbox, "", 0.0
def detect_plate_and_ocr(image_bytes: bytes) -> tuple[BBox, str, float]:
    # Try real models first, fallback to dummy
    yolo = get_yolo_model()
    ocr = get_easyocr()
    if yolo is None or ocr is None:
        return dummy_detect_and_ocr(image_bytes)
    img = Image.open(io.BytesIO(image_bytes)).convert("RGB")
    np_img = np.array(img)
    # YOLO inference
    try:
        results = yolo(np_img, verbose=False)
        if not results or len(results[0].boxes) == 0:
            return dummy_detect_and_ocr(image_bytes)
        # Take the highest confidence box
        boxes = results[0].boxes
        confs = boxes.conf.cpu().numpy()
        xyxy = boxes.xyxy.cpu().numpy()
        idx = int(np.argmax(confs))
        x1, y1, x2, y2 = xyxy[idx]
        w = float(max(1.0, x2 - x1))
        h = float(max(1.0, y2 - y1))
        bbox = BBox(x=float(x1), y=float(y1), w=w, h=h)
        crop = np_img[int(y1):int(y2), int(x1):int(x2)]
        # OCR
        result = ocr.readtext(crop)
        if not result:
            return bbox, "", float(confs[idx])
        # choose the text with max confidence
        text_item = max(result, key=lambda r: r[2])
        plate_text = text_item[1]
        confidence = float(min(1.0, max(0.0, (confs[idx] + text_item[2]) / 2)))
        return bbox, plate_text, confidence
    except Exception:
        return dummy_detect_and_ocr(image_bytes)


def detect_vehicle_color(image_bytes: bytes, bbox: Optional[BBox]) -> Optional[str]:
    try:
        img = Image.open(io.BytesIO(image_bytes)).convert("RGB")
        np_img = np.array(img)
        if bbox is not None:
            x1, y1 = int(max(0, bbox.x)), int(max(0, bbox.y))
            x2, y2 = int(min(np_img.shape[1], bbox.x + bbox.w)), int(min(np_img.shape[0], bbox.y + bbox.h))
            crop = np_img[y1:y2, x1:x2]
        else:
            crop = np_img
        if crop.size == 0:
            return None
        import cv2
        hsv = cv2.cvtColor(crop, cv2.COLOR_RGB2HSV)
        hue = hsv[:, :, 0].astype(np.float32)
        sat = hsv[:, :, 1].astype(np.float32)
        val = hsv[:, :, 2].astype(np.float32)
        mean_h = float(np.mean(hue))
        mean_s = float(np.mean(sat))
        mean_v = float(np.mean(val))
        if mean_v < 60:
            return "مشکی"
        if mean_s < 30 and mean_v > 200:
            return "سفید"
        # Rough hue buckets
        if 0 <= mean_h < 10 or 170 <= mean_h <= 180:
            return "قرمز"
        if 10 <= mean_h < 25:
            return "نارنجی"
        if 25 <= mean_h < 35:
            return "زرد"
        if 35 <= mean_h < 85:
            return "سبز"
        if 85 <= mean_h < 130:
            return "آبی"
        if 130 <= mean_h < 160:
            return "بنفش"
        return "نقره‌ای"
    except Exception:
        return None


def anonymize_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()[:16]


def maybe_sync_to_cloud(result: "LPRResult") -> None:
    if not CLOUD_SYNC_ENABLED:
        return
    try:
        payload = result.model_dump()
        # anonymize sensitive fields
        payload["plate_text"] = anonymize_text(payload.get("plate_text", ""))
        payload["camera_id"] = anonymize_text(payload.get("camera_id", ""))
        headers = {"Content-Type": "application/json"}
        if CLOUD_API_TOKEN:
            headers["Authorization"] = f"Bearer {CLOUD_API_TOKEN}"
        requests.post(f"{CLOUD_API_URL}/ingest/lpr", headers=headers, json=payload, timeout=5)
    except Exception:
        # Best-effort only; ignore network errors
        pass


@app.post("/infer", response_model=LPRResult)
async def infer(
    image: UploadFile = File(...),
    camera_id: str = Form(...),
):
    image_bytes = await image.read()
    bbox, plate_text, confidence = detect_plate_and_ocr(image_bytes)
    vehicle_color = detect_vehicle_color(image_bytes, bbox)

    ts = datetime.now(timezone.utc).isoformat()
    edge_image_path = None

    result = LPRResult(
        timestamp=ts,
        camera_id=camera_id,
        plate_text=plate_text,
        confidence=confidence,
        bbox=bbox,
        vehicle_color=vehicle_color,
        vehicle_type=None,
        edge_image_path=edge_image_path,
    )

    write_log(result.model_dump())
    if confidence >= AI_CONF_THRESHOLD:
        maybe_sync_to_cloud(result)
    return JSONResponse(content=result.model_dump())


class LatestQuery(BaseModel):
    limit: int = 20


@app.get("/results/latest", response_model=List[LPRResult])
async def results_latest(limit: int = 20):
    if not LOG_FILE.exists():
        return []
    results: List[LPRResult] = []
    with LOG_FILE.open("r", encoding="utf-8") as f:
        lines = f.readlines()
    for line in lines[-limit:]:
        try:
            obj = orjson.loads(line)
            results.append(LPRResult(**obj))
        except Exception:
            continue
    return results


@app.get("/healthz")
async def healthz():
    return {"status": "ok"}




