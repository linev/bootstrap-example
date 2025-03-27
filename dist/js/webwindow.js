import { connectWebWindow } from 'jsroot/webwindow';
import { TCanvasPainter } from 'jsroot';

let conn_handle = null;

function addOutput(msg) {
   const elem = document.body.querySelector('#rootapp_debug_output');
   if (elem)
      elem.innerHTML = msg;
   else
      console.log('GOT MESSAGE', msg);
}

// add handler for the button click
let cnt = 0;
const btn = document.body.querySelector('#root_ping_button');
btn?.addEventListener('click', () => conn_handle.send(cnt++ % 2 ? 'get_text' : 'get_binary'));

function addCanvasToMainPage(handle) {

   // create channel which need to be accepted by the server
   const conn = handle.createChannel();

   const dom = document.body.querySelector('#rootapp_canvas');

   const painter = new TCanvasPainter(dom, null);

   painter.online_canvas = true; // indicates that canvas gets data from running server
   painter.embed_canvas = true;  // use to indicate that canvas ui should not close complete window when closing
   painter.use_openui = false;   // use by default ui5 widgets

   // let canvas use create channel
   painter.useWebsocket(conn);
   handle.send('channel:' + conn.getChannelId());
}

connectWebWindow({
   receiver: {
      // method called when connection to server is established
      onWebsocketOpened(handle) {
            handle.send("Connection established");
            conn_handle = handle;
            addOutput("Connected");

            addCanvasToMainPage(handle);
      },

      // method with new message from server
      onWebsocketMsg(handle, msg, offset) {
            if (typeof msg != "string") {
               let arr = new Float32Array(msg, offset);
               addOutput("bin: " + arr.toString());
            } else {
               addOutput("txt: " + msg);
            }
      },

      // method called when connection is gone
      onWebsocketClosed(handle) {
            conn_handle = null;
            // when connection closed, close panel as well
            if (window) window.close();
      }
   }
});