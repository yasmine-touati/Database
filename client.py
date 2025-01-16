import socket
import pandas as pd
import sys
import json
import time
import os

def send_request(method, path, body=None):
    request = f"{method} {path} HTTP/1.1\r\nHost: localhost\r\n"
    if body:
        body_json = json.dumps(body)
        request += f"Content-Length: {len(body_json)}\r\n"
        request += "Content-Type: application/json\r\n"
        request += "\r\n" + body_json
    else:
        request += "\r\n"

    print(f"Sending request: {request}")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('localhost', 6667))
        s.sendall(request.encode())
        
        response = b""
        while True:
            chunk = s.recv(8192)
            if not chunk:
                break
            response += chunk

    response = response.decode()
    
    try:
        body = response
        return json.loads(body)
    except (ValueError, IndexError, json.JSONDecodeError) as e:
        print(f"Failed to parse response: {e}")
        return response

def main(csv_path):
    # Read CSV file
    csv = []
    with open(csv_path, 'r', encoding="latin1") as f:
        csv = f.readlines()
    num_rows = len(csv)
    dataset_name = os.path.splitext(os.path.basename(csv_path))[0]
    b_plus_tree_order = max(2, num_rows // 4)

    # Create dataset
    print(f"\n=== Creating dataset '{dataset_name}' with order {b_plus_tree_order} ===")
    response = send_request('POST', f'/dataset/{dataset_name}/create/order/{b_plus_tree_order}')
    print(json.dumps(response, indent=2))
    
    time.sleep(1)
    
    # Insert CSV lines in batches of 100
    for i in range(0, num_rows, 10):
        batch = csv[i:i+10]
        bulk_data = {
            "entries": [{"key": idx+i, "line": row.strip()} for idx, row in enumerate(batch)]
        }
        print(f"\n=== Inserting batch {i//10 + 1} ===")
        print(f"Bulk data: {json.dumps(bulk_data, indent=2)}")
        response = send_request('POST', f'/dataset/{dataset_name}/bulk', bulk_data)
        print(json.dumps(response, indent=2))
        time.sleep(1)
    
    # Test other endpoints
    print(f"\n=== Range query from 5 to 15 ===")
    response = send_request('GET', f'/dataset/{dataset_name}/range/start/5/end/15')
    print(json.dumps(response, indent=2))
    
    for key in [5, 7, 13]:
        print(f"\n=== Deleting key {key} ===")
        response = send_request('DELETE', f'/dataset/{dataset_name}/key/{key}')
        print(json.dumps(response, indent=2))
        time.sleep(0.5)
    
    for key in [6, 7, 8]:
        print(f"\n=== Searching for key {key} ===")
        response = send_request('GET', f'/dataset/{dataset_name}/search/key/{key}')
        print(json.dumps(response, indent=2))
        time.sleep(0.5)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python client.py <path_to_csv_file>")
        sys.exit(1)
    
    csv_path = sys.argv[1]
    main(csv_path)