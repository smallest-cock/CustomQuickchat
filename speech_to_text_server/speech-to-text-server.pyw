#! /usr/bin/env pythonw

import logging
import os
import sys

# log errors to 'ErrorLog.txt' in same directory as script
executable_folder = os.path.dirname(sys.argv[0])
log_file = os.path.join(executable_folder, 'ErrorLog.txt')
logging.basicConfig(filename=log_file, level=logging.INFO, 
                    format='%(asctime)s [%(levelname)s] %(message)s')

try:
    import asyncio
    import websockets
    import argparse
    import json
    import psutil
    import speech_recognition as sr
except ImportError as e:
    logging.error(f"Failed to import module: {e}")
    sys.exit(1)


# modify argparse to add logging before it exits on an error
class ArgumentParserWithLogging(argparse.ArgumentParser):
    def error(self, message):
        logging.error(f"Argument parsing error: {message}")
        super().error(message)  # prints the error and raises SystemExit


def port_in_use(port: int) -> bool:
  for connection in psutil.net_connections():
    if connection.laddr.port == port:
      return True
  return False


class WebsocketHandler:
    def __init__(self, websocket, port_num):
        self.websocket = websocket
        self.portNum = port_num

    # --------------------- sending/formatting responses ---------------------

    async def send_response(self, response: dict):
        await self.websocket.send(json.dumps(response))

    def format_response(self, event_name: str, data: dict, attempt_id: str = "") -> dict:
        if attempt_id:
            data["attemptId"] = attempt_id

        return { "event": event_name, "data": data }
    
    def error_response(self, errorMsg: str, attempt_id: str = "") -> dict:
        return self.format_response("error_response", { "errorMsg": errorMsg }, attempt_id)
    
    # ------------------------------------------------------------------------


    async def handle_message(self, message):
        try:
            request : dict = json.loads(message)    # parse the message as JSON
            print(f"Received request: {request}")

            event = request.get("event")
            data = request.get("data")
            
            if event is None or data is None:
                response = self.error_response("Invalid JSON in websocket message")
            else:
                response = await self.get_response_from_event(event, data)

            await self.send_response(response)  # send reponse back to client as serialized JSON

        except json.JSONDecodeError:    # Handle invalid JSON
            response = self.error_response("Invalid JSON format")
            await self.send_response(response)


    async def get_response_from_event(self, event: str, request_data: dict) -> dict:
        response = self.error_response("Invalid event name in websocket request JSON")

        if event == "start_speech_to_text":
            response = await self.process_STT_request(request_data)

        elif event == "calibrate_microphone":
            response = await self.process_calibration_request(request_data)

        elif event == "test":
            response = self.format_response("test_response", {"message": f"This is a response from the websocket server listening on port {self.portNum} :)"})

        return response


    async def process_STT_request(self, request_data: dict):
        response = self.error_response("Unable to parse the STT request data")  # default response

        if stt_args := request_data.get("args"):
            if attempt_id := stt_args.get("attemptId"):
                stt_result = await self.speech_to_text(
                    stt_args["beginSpeechTimeout"],
                    stt_args["processSpeechTimeout"],
                    stt_args["autoCalibrateMic"],
                    stt_args["micEnergyThreshold"],
                    attempt_id)
                
                response = self.format_response("speech_to_text_result", stt_result, attempt_id)

        return response

    async def process_calibration_request(self, data: dict) -> dict:
        response = self.error_response("Unable to parse the mic calibration request data")  # default response

        if attempt_id := data.get("attemptId"):
            calibration_result = await self.calibrate_microphone(attempt_id)
            response = self.format_response("mic_calibration_result", calibration_result, attempt_id)

        return response
    

    async def speech_to_text(self,
        start_speech_timeout:       float,
        process_request_timeout:    float,
        auto_calibrate_mic:         bool,
        mic_energy_threshold:       float,
        attempt_id:                 str
        ) -> dict:
            try:
                if mic is None:
                    return { "success": False, "errorMsg": "Microphone initialization failed. Make sure mic access is enabled" }
                with mic as source:
                    print("Listening for audio...")
                    await self.send_response(self.format_response("notify_mic_listening", {}, attempt_id))    # notify client mic is listening...

                    # set mic energy threshold
                    if auto_calibrate_mic:
                        recognizer.adjust_for_ambient_noise(source)
                    else:
                        recognizer.energy_threshold = mic_energy_threshold

                    audio = recognizer.listen(source, timeout=start_speech_timeout, phrase_time_limit=process_request_timeout)
                    result = recognizer.recognize_google(audio)
                    
                    return { "success": True, "transcription": result }
            
            except sr.WaitTimeoutError:
                return { "success": False, "errorMsg": "Timeout: No speech detected" }
            except sr.UnknownValueError:
                return { "success": False, "errorMsg": "Could not understand audio" }
            except sr.RequestError as e:
                return { "success": False, "errorMsg": f"API request failed: {str(e)}" }
            except Exception as e:
                return { "success": False, "errorMsg": str(e) }


    async def calibrate_microphone(self, attempt_id: str) -> dict:
        try:
            print(f'BEFORE: energy_threshold = {recognizer.energy_threshold}')

            # notify client that mic is listening
            await self.send_response(self.format_response("notify_mic_listening", {}, attempt_id))

            if mic is None:
                return { "success": False, "errorMsg": "Microphone initialization failed. Make sure mic access is enabled" }

            with mic as source:
                recognizer.adjust_for_ambient_noise(source)

            print(f'AFTER: energy_threshold = {recognizer.energy_threshold}')

            return { "success" : True, "mic_energy_threshold" : recognizer.energy_threshold }

        except OSError as e:
            return { "success" : False, "errorMsg" : "No microphone detected" }
        except Exception as e:
            return { "success" : False, "errorMsg" : str(e) }



# handle incoming websocket connections
async def handle_client(websocket, stop_event, port_num):
    ws_connection = WebsocketHandler(websocket, port_num)
    logging.info("Client connected")

    try:
        async for message in websocket:
            await ws_connection.handle_message(message)

    except websockets.ConnectionClosed:
        logging.info("Client disconnected... shutting down.")
    except Exception as e:
        logging.error(f"Unexpected error: {e}")

    stop_event.set()  # Graceful shutdown even on unexpected errors


async def start_server(port: int):
    if port_in_use(port):
        logging.error(f"Unable to start WebSocket server on port {port}! It's already in use")
        sys.exit(1)

    stop_event = asyncio.Event()

    async def client_wrapper(websocket):
        await handle_client(websocket, stop_event, port)

    logging.info(f"Starting WebSocket server on port {port} ...")

    # server = await websockets.serve(client_wrapper, "localhost", port, reuse_address=True)

    try:
        server = await websockets.serve(
            client_wrapper,
            "localhost",
            port,
            reuse_address=True,  # allow immediate reuse after disconnect
        )
    except OSError as e:
        logging.error(f"Failed to bind WebSocket server on port {port}: {e}")
        sys.exit(1)  # bail out cleanly

    await stop_event.wait()     # Wait until stop_event is set by handle_client()

    logging.info("Stopping server...")
    server.close()
    await server.wait_closed()
    logging.info("Server shut down successfully.")



def main():
    parser = ArgumentParserWithLogging(description="A script to start a websocket server and convert speech to text based on requests from another process")
    
    # positional arguments
    parser.add_argument('port_number', type=int, help='Port number used to start websocket server')

    # parse arguments
    args = parser.parse_args()

    asyncio.run(start_server(args.port_number))



# global variables (so a new object isn't created on every client request)
recognizer = sr.Recognizer()
mic = sr.Microphone()


if __name__ == "__main__":
    main()
