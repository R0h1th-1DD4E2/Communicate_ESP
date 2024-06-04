document.addEventListener("DOMContentLoaded", () => {
    let ws;

    const connectBtn = document.getElementById("connect-btn");
    const ipAddressInput = document.getElementById("ip-address");

    connectBtn.addEventListener("click", () => {
        const ipAddress = ipAddressInput.value.trim();
        if (ipAddress) {
            const wsUrl = `ws://${ipAddress}:8765`;
            ws = new WebSocket(wsUrl);

            ws.onopen = () => {
                console.log(`Connected to WebSocket server at ${wsUrl}`);
                ws.send("Client connected");
            };

            ws.onmessage = (event) => {
                const messageBox = document.getElementById("messages");
                messageBox.innerHTML += `<p>${event.data}</p>`;
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
