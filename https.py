import requests
import urllib3

# Suppress InsecureRequestWarning
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

def send_https_request(url, method="GET", data=None, headers=None):
    try:
        if method.upper() == "GET":
            response = requests.get(url, headers=headers, verify=False)
        elif method.upper() == "POST":
            response = requests.post(url, data=data, headers=headers, verify=False)
        elif method.upper() == "PUT":
            response = requests.put(url, data=data, headers=headers, verify=False)
        elif method.upper() == "DELETE":
            response = requests.delete(url, headers=headers, verify=False)
        else:
            print(f"Unsupported HTTP method: {method}")
            return

        print(f"Response Status Code: {response.status_code}")
        print(f"Response Body: {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    url = "http://localhost:8080"  # Fixed URL
    method = input("Enter the HTTP method (GET, POST, PUT, DELETE): ").strip().upper()
    endpoint = input("Enter the endpoint (leave empty for root): ").strip()
    if endpoint:
        url += f"/{endpoint.lstrip('/')}"
    data = None
    if method in ["POST", "PUT"]:
        data = input("Enter the data to send (as key=value pairs, separated by &): ")
    headers = {"Content-Type": "application/x-www-form-urlencoded"}

    send_https_request(url, method, data, headers)