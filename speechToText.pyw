#! /usr/bin/env pythonw

import logging
import os
import sys

# log errors to 'ErrorLog.txt' in same directory as script
log_file = os.path.join(os.path.dirname(sys.argv[0]), 'ErrorLog.txt')
logging.basicConfig(filename=log_file, level=logging.ERROR, 
                    format='%(asctime)s [%(levelname)s] %(message)s')

try:
    import pyaudio		# just to make sure pyaudio module is installed
    import speech_recognition as sr
    import argparse
    import json
except ImportError as e:
    logging.error(f"Failed to import module: {e}")
    sys.exit(1)


# modify argparse to add logging before it exits
class ArgumentParserWithLogging(argparse.ArgumentParser):
    def error(self, message):
        logging.error(f"Argument parsing error: {message}")
        super().error(message)  # prints the error and raises SystemExit


def main():

    parser = ArgumentParserWithLogging(description="A script to convert speech to text, and save the result in a JSON file.")
    
    # positional arguments
    parser.add_argument('json_file', type=str, help='File path to SpeechToText.json')
    parser.add_argument('listening_timeout', type=float, help='Max amount of time to wait for user to start speech')
    parser.add_argument('attempt_ID', type=str, help='Unique ID representing the current speech-to-text attempt')

    # optional arguments
    parser.add_argument('--calibrate', action='store_true', help='Flag to calibrate mic for ambient noise instead of listening for speech')

    # parse arguments
    args = parser.parse_args()

    if args.calibrate:
        calibrate_mic(args)
    else:
        transcription = get_transcription(args)

        if transcription:
            with open(args.json_file, 'r+') as f:
                data = json.load(f)
                data['transcription']['ID'] = args.attempt_ID
                # data['transcription']['time'] = current_time 
                data['transcription']['text'] = transcription
                data['transcription']['error'] = False
                f.seek(0)        # go to 1st line of file
                json.dump(data, f, indent=4)
                f.truncate()     # remove remaining part
    

def get_transcription(args: argparse.Namespace) -> str | None:
    try:
        recognizer = sr.Recognizer()
        mic = sr.Microphone()

        with open(args.json_file, 'r') as f:
            data = json.load(f)
            if 'micNoiseCalibration' in data:
                recognizer.energy_threshold = data['micNoiseCalibration']

        with mic as source:
            audio = recognizer.listen(source, timeout=args.listening_timeout)    # listen within time limit for the start of speech

        transcription = recognizer.recognize_google(audio)
        return transcription.lower() if transcription else None

    except OSError as e:
        logging.error(f"ERROR: {e}")
        write_error_to_json(args, 'No mic detected...')
    except sr.WaitTimeoutError as e:
        logging.error(f"ERROR: {e}")
        # no speech detected
        default_device_name = pyaudio.PyAudio().get_default_input_device_info()['name'] 
        write_error_to_json(args, f"No speech detected from '{default_device_name}'. Make sure it's not muted!")
    except sr.RequestError as e:
        logging.error(f"ERROR: {e}")
        # API was unreachable or unresponsive
        write_error_to_json(args, 'Google Speech Recognition API is unavailable')
    except sr.UnknownValueError as e:
        logging.error(f"ERROR: {e}")
        # speech was unintelligible
        write_error_to_json(args, 'Unable to recognize speech')
    except Exception as e:
        logging.error(f"ERROR: {e}")
        write_error_to_json(args, str(e))

    
def calibrate_mic(args: argparse.Namespace):
    try:
        recognizer = sr.Recognizer()
        mic = sr.Microphone()

        with mic as source:
            recognizer.adjust_for_ambient_noise(source=source, duration=2)

        with open(args.json_file, 'r+') as f:
            data = json.load(f)
            data['micNoiseCalibration'] = recognizer.energy_threshold
            f.seek(0)        # go to 1st line
            json.dump(data, f, indent=4)
            f.truncate()     # remove remaining part

    except OSError as e:
        logging.error(f"ERROR: {e}")
        write_error_to_json(args, 'No mic detected...')
    except Exception as e:
        logging.error(f"ERROR: {e}")
        write_error_to_json(args, str(e))


def write_error_to_json(args: argparse.Namespace, error_message: str):
    with open(args.json_file, 'r+') as f:
        data = json.load(f)
        data['transcription']['ID'] = args.attempt_ID
        data['transcription']['error'] = True
        data['transcription']['errorMessage'] = error_message
        f.seek(0)        # go to 1st line of file
        json.dump(data, f, indent=4)
        f.truncate()     # remove remaining part


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        logging.error(f"ERROR: {e}")
        sys.exit(1)
