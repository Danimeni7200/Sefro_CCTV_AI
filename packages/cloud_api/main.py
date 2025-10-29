from datetime import datetime, timedelta, timezone
from typing import Optional, List
from fastapi import WebSocket, WebSocketDisconnect
import asyncio

from fastapi import FastAPI, HTTPException, Depends
from fastapi.middleware.cors import CORSMiddleware
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from pydantic import BaseModel
import jwt
import os
import orjson
from sqlmodel import SQLModel, Field, create_engine, Session, select
from passlib.context import CryptContext


JWT_SECRET = os.getenv("JWT_SECRET", "dev_secret_change")
JWT_ALG = "HS256"
TOKEN_EXPIRE_MIN = 60 * 24

app = FastAPI(title="Cloud API")

# CORS for local Next.js dev and Electron
allowed_origins = [
    "http://localhost:3000",
    "http://127.0.0.1:3000",
    "http://localhost",
    "http://127.0.0.1",
]
app.add_middleware(
    CORSMiddleware,
    allow_origins=allowed_origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"]
)
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="/auth/token")


class Token(BaseModel):
    access_token: str
    token_type: str = "bearer"


class User(BaseModel):
    username: str


def create_token(username: str) -> str:
    now = datetime.now(timezone.utc)
    payload = {
        "sub": username,
        "iat": int(now.timestamp()),
        "exp": int((now + timedelta(minutes=TOKEN_EXPIRE_MIN)).timestamp()),
    }
    return jwt.encode(payload, JWT_SECRET, algorithm=JWT_ALG)


def get_current_user(token: str = Depends(oauth2_scheme)) -> User:
    try:
        payload = jwt.decode(token, JWT_SECRET, algorithms=[JWT_ALG])
        return User(username=payload["sub"])
    except Exception as e:
        raise HTTPException(status_code=401, detail="Invalid token")


# -----------------------
# Database-backed users
# -----------------------
class UserDB(SQLModel, table=True):
    id: Optional[int] = Field(default=None, primary_key=True)
    username: str = Field(index=True, unique=True)
    password_hash: str
    api_token: str = Field(index=True, unique=True)
    is_active: bool = True


pwd_context = CryptContext(schemes=["pbkdf2_sha256"], deprecated="auto")
DATABASE_URL = os.getenv("DATABASE_URL", "sqlite:///./cloud_lpr.db")
engine = create_engine(DATABASE_URL, echo=False)


def init_db() -> None:
    SQLModel.metadata.create_all(engine)
    with Session(engine) as session:
        existing = session.exec(select(UserDB).where(UserDB.username == "admin")).first()
        if not existing:
            user = UserDB(
                username="admin",
                password_hash=pwd_context.hash("admin123"),
                api_token="DEV-TOKEN-CHANGE",
                is_active=True,
            )
            session.add(user)
            session.commit()


init_db()


@app.post("/auth/token", response_model=Token)
def login(form_data: OAuth2PasswordRequestForm = Depends()):
    # Expect password formatted as "<password>::<api_token>"
    if not form_data.username or not form_data.password:
        raise HTTPException(status_code=400, detail="Username and password required")
    if "::" not in form_data.password:
        raise HTTPException(status_code=400, detail="API token required")
    plain_pw, api_token = form_data.password.split("::", 1)

    with Session(engine) as session:
        user = session.exec(select(UserDB).where(UserDB.username == form_data.username)).first()
        if not user or not user.is_active:
            raise HTTPException(status_code=401, detail="Invalid credentials")
        if not pwd_context.verify(plain_pw, user.password_hash):
            raise HTTPException(status_code=401, detail="Invalid credentials")
        if api_token != user.api_token:
            raise HTTPException(status_code=401, detail="Invalid API token")

    token = create_token(form_data.username)
    return Token(access_token=token)


class LPRResult(BaseModel):
    timestamp: str
    camera_id: str
    plate_text: str
    confidence: float
    bbox: dict
    vehicle_color: Optional[str] = None
    vehicle_type: Optional[str] = None
    edge_image_path: Optional[str] = None


@app.post("/ingest/lpr")
def ingest_lpr(result: LPRResult, user: User = Depends(get_current_user)):
    # Placeholder: simply echo, future: store and trigger analytics
    return {"status": "received", "by": user.username}


@app.get("/analytics/summary")
def analytics_summary(user: User = Depends(get_current_user)):
    # Placeholder static summary
    return {
        "total_plates": 0,
        "by_camera": {},
        "top_confidence": [],
    }


@app.get("/healthz")
def healthz():
    return {"status": "ok"}


clients: List[WebSocket] = []


@app.websocket("/ws/analytics")
async def ws_analytics(websocket: WebSocket):
    await websocket.accept()
    clients.append(websocket)
    try:
        while True:
            await asyncio.sleep(2)
            await websocket.send_json({"ts": int(datetime.now(timezone.utc).timestamp()), "total_plates": 0})
    except WebSocketDisconnect:
        if websocket in clients:
            clients.remove(websocket)


