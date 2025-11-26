# Upload & Download Test Server

This Flask server provides two endpoints used to test HTTP throughput.

### 1. Download test
`GET /testfile.bin`

Returns a 1 MiB binary file for measuring download speed.  
If the file does not exist, it is generated automatically at startup.

### 2. Upload test
`POST /upload`

Accepts raw binary data and returns:
- number of bytes received  
- transfer duration  
- calculated upload speed (Mbps)

Example response:
```json
{
  "size": 1048576,
  "duration_s": 0.11,
  "mbps": 76.32
}
