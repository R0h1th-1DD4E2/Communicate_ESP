document.addEventListener("DOMContentLoaded", () => {
    let ws;

    const connectBtn = document.getElementById("connect-btn");
    const ipAddressInput = document.getElementById("ip-address");

    connectBtn.addEventListener("click", () => {
        const ipAddress = ipAddressInput.value.trim();
        if (ipAddress) {
            const wsUrl = `ws://${ipAddress}:8080`;
            ws = new WebSocket(wsUrl);

            ws.onopen = () => {
                console.log(`Connected to WebSocket server at ${wsUrl}`);
                ws.send("Client connected");
            };

            ws.onmessage = (event) => {
                const messageBox = document.getElementById("messages-content");
                const data = JSON.parse(event.data);
                
                console.log("Incoming Data: ", data);

                if (data.type === "Error") {
                    messageBox.innerHTML += `<p style="color:red;">${data.message}</p>`;
                }
                else if (data.type === "Discover") {
                    messageBox.innerHTML += `<p style="color:orange;">${data.message}</p>`;
                }
                else if (data.type === "Register") {
                    messageBox.innerHTML += `<p style="color:yellow;">${data.message}</p>`;
                }
                else if (data.type === "NoAckReq") {
                    messageBox.innerHTML += `<p style="color:green;">${data.message}</p>`;
                }
                else if (data.type === "ack") {
                    messageBox.innerHTML += `<p style="color:green;">${data.message}</p>`;
                }
                else if (data.type === "Update") {
                    messageBox.innerHTML += `<p style="color:green;">${data.message}</p>`;
                }
                else if (data.type === "Device" && data.message) {
                    const devicesBox = document.getElementById("devices-content");
                    devicesBox.innerHTML = ""; // Clear previous list
                    const macAddresses = JSON.parse(data.message);
                    macAddresses.forEach(mac => {
                        devicesBox.innerHTML += `<p>${mac}</p>`;
                    });
                }
                else {}
            };
            ws.onclose = () => {
                console.log("Disconnected from WebSocket server");
            };

            ws.onerror = (error) => {
                console.error("WebSocket error: ", error);
            };
        } else {
            alert("Please enter a valid IP address.");
        }
    });

    const themeToggleBtn = document.getElementById("theme-toggle");
    themeToggleBtn.addEventListener("click", () => {
        document.body.classList.toggle("dark-mode");
    });
});
