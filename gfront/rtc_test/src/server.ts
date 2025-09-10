import path from "path";
import express from "express";
import http from "http";
import { Server, Socket } from "socket.io";

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// ✅ serve static files from src/public
app.use(express.static(path.join(__dirname, "public")));

io.on("connection", (socket: Socket) => {
  console.log("A user connected");

  socket.on("message", (message: string) => {
    console.log("Received message:", message);
    io.emit("message", message);
  });

  socket.on("disconnect", () => {
    console.log("A user disconnected");
  });
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
