import requests
import json

def test_cpp_discovery():
    url = "http://127.0.0.1:8086/discover"
    params = {
        "ip": "192.168.4.252",
        "user": "admin",
        "pass": "test1234",
        "brand": "reolink"
    }
    
    try:
        response = requests.post(url, params=params, timeout=10)
        print(f"Status Code: {response.status_code}")
        print(f"Response: {response.text}")
        return response.json()
    except Exception as e:
        print(f"Error: {e}")
        return None

if __name__ == "__main__":
    print("Testing C++ Discovery Service...")
    result = test_cpp_discovery()
    if result:
        print("Success!")
        print(json.dumps(result, indent=2))
    else:
        print("Failed to connect to C++ Discovery Service")