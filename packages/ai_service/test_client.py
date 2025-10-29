import requests

# Test health endpoint
response = requests.get("http://127.0.0.1:8000/healthz")
print("Health check response:", response.json())

# Test inference endpoint
with open("test_image.jpg", "rb") as f:
    files = {"image": ("test_image.jpg", f, "image/jpeg")}
    data = {"camera_id": "CAM01"}
    response = requests.post("http://127.0.0.1:8000/infer", files=files, data=data)
    print("Inference response:", response.json())