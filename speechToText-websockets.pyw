#! /usr/bin/env pythonw

import logging
import os
import sys

# log errors to 'ErrorLog.txt' in same directory as script
executable_folder = os.path.dirname(sys.argv[0])
log_file = os.path.join(executable_folder, 'ErrorLog.txt')
logging.basicConfig(filename=log_file, level=logging.ERROR, 
                    format='%(asctime)s [%(levelname)s] %(message)s')


try:
    import asyncio
    import websockets
    import argparse
    import json
    import pyaudio		# just to make sure pyaudio module is installed
    import speech_recognition as sr
except ImportError as e:
    logging.error(f"Failed to import module: {e}")
    sys.exit(1)


# modify argparse to add logging before it exits on an error
class ArgumentParserWithLogging(argparse.ArgumentParser):
    def error(self, message):
        logging.error(f"Argument parsing error: {message}")
        super().error(message)  # prints the error and raises SystemExit


def create_STT_result_dict(data : dict, attempt_id : str) -> dict:
    data["attemptId"] = attempt_id
    return {"event": "speech_to_text_result", "data": data}


def process_speech_to_text(start_speech_timeout : float,
                        process_request_timeout : float,
                        auto_calibrate_mic : bool,
                        mic_energy_threshold : float,
                        attempt_ID : str):
    recognizer = sr.Recognizer()
    with sr.Microphone() as source:
        print("Listening for audio...")
        try:
            # set mic energy threshold
            if auto_calibrate_mic:
                recognizer.adjust_for_ambient_noise()
            else:
                recognizer.energy_threshold = mic_energy_threshold

            audio = recognizer.listen(source, timeout=start_speech_timeout, phrase_time_limit=process_request_timeout)
            result = recognizer.recognize_google(audio)
            
            return create_STT_result_dict({"success": True, "transcription": result}, attempt_ID)
        
        except sr.WaitTimeoutError:
            return create_STT_result_dict({"success": False, "errorMsg": "Timeout: No speech detected"}, attempt_ID)
        except sr.UnknownValueError:
            return create_STT_result_dict({"success": False, "errorMsg": "Could not understand audio"}, attempt_ID)
        except sr.RequestError as e:
            return create_STT_result_dict({"success": False, "errorMsg": f"API request failed: {e}"}, attempt_ID)
        except Exception as e:
            return create_STT_result_dict({"success": False, "errorMsg": e}, attempt_ID)


# handle incoming websocket connections
async def handle_client(websocket):
    print("Client connected")
    try:
        async for message in websocket:
            try:
                # Parse the incoming message as JSON
                request : dict = json.loads(message)
                print(f"Received request: {request}")

                # Check the type of request based on JSON content
                if request.get("event") == "start_speech_to_text":
                    if data := request.get("data"):
                        if stt_args := data.get("args"):
                            response = process_speech_to_text(
                                stt_args["beginSpeechTimeout"],
                                stt_args["processSpeechTimeout"],
                                stt_args["autoCalibrateMic"],
                                stt_args["attemptId"])
                    await websocket.send(json.dumps(response))

                elif request.get("event") == "test":
                    response = {"testResponse": {"message": "this is a message from the other side"}}
                    await websocket.send(json.dumps(response))

                else:
                    await websocket.send(json.dumps({"status": "error", "message": "Unknown command"}))

            except json.JSONDecodeError:
                # Handle invalid JSON
                error_response = {"status": "error", "message": "Invalid JSON format"}
                await websocket.send(json.dumps(error_response))

    except websockets.ConnectionClosed:
        print("Client disconnected")
    except Exception as e:
        logging.error(f"Error: {e}")


async def start_server(port : int):
    print(f"Starting WebSocket server using port {port} ...")
    server = await websockets.serve(handle_client, "localhost", port)
    await server.wait_closed()


def main():
    parser = ArgumentParserWithLogging(description="A script to start a websocket server and convert speech to text based on requests from another process")
    
    # positional arguments
    parser.add_argument('port_number', type=int, help='Port number used to start websocket server')

    # parse arguments
    args = parser.parse_args()

    asyncio.run(start_server(args.port_number))


if __name__ == "__main__":
    main()
