#pragma once
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>ESP32 IP Camera</title>
  <style>
    * { box-sizing: border-box; }
    html, body {
      margin: 0;
      padding: 0;
      height: 100%;
      width: 100%;
      font-family: Arial, sans-serif;
      background-color: #f2f4f8;
      overflow-x: hidden;
    }
    body {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      padding: 1.5rem;
    }
    h1 {
      color: #333;
      margin-bottom: 1.5rem;
      font-size: 1.6rem;
      text-align: center;
    }
    .form-container {
      background-color: white;
      padding: 1.2rem;
      border-radius: 12px;
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
      width: 100%;
      max-width: 400px;
    }
    label {
      display: block;
      margin-bottom: 0.3rem;
      font-weight: bold;
      color: #555;
    }
    input {
      width: 100%;
      padding: 0.7rem;
      margin-bottom: 1rem;
      border: 1px solid #ccc;
      border-radius: 8px;
      font-size: 1rem;
    }
    button {
      width: 100%;
      padding: 0.9rem;
      background-color: #007bff;
      color: white;
      border: none;
      border-radius: 8px;
      font-size: 1rem;
      cursor: pointer;
      transition: background-color 0.2s;
    }
    button:hover {
      background-color: #0056b3;
    }
    @media (max-width: 480px) {
      body { padding: 1rem; }
      h1 { font-size: 1.4rem; }
      .form-container { padding: 1rem; }
    }
  </style>
</head>
<body>
  <h1>ESP32 IP Camera</h1>
  <div class="form-container">
    <form id="cameraForm">
      <label for="username">Username</label>
      <input type="text" id="username" placeholder="Enter username" required />

      <label for="password">Password</label>
      <input type="password" id="password" placeholder="Enter password" required />

      <label for="deviceIp">Camera IP Address</label>
      <input type="text" id="deviceIp" placeholder="192.168.0.100" required />

      <button type="submit">Connect</button>
    </form>
  </div>

  <script>
    document.getElementById("cameraForm").addEventListener("submit", function (e) {
      e.preventDefault();

      const data = {
        username: document.getElementById("username").value,
        password: document.getElementById("password").value,
        deviceIp: document.getElementById("deviceIp").value
      };

      fetch("/settings", {
        method: "POST",
        headers: {
          "Content-Type": "application/x-www-form-urlencoded"
        },
        body: new URLSearchParams(data)
      })
      .then(response => {
        if (response.ok) {
          return response.text();
        } else {
          throw new Error("Yêu cầu lỗi");
        }
      })
      .then(text => {
        alert("Cấu hình thành công!");
      })
      .catch(err => {
        alert("Lỗi: " + err.message);
      });
    });
  </script>
</body>
</html>
)rawliteral";
