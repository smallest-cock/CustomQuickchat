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
    import pyaudio		# just to make sure pyaudio module is installed
    import speech_recognition as sr
    import argparse
    import json
except ImportError as e:
    logging.error(f"Failed to import module: {e}")
    sys.exit(1)


# modify argparse to add logging before it exits on an error
class ArgumentParserWithLogging(argparse.ArgumentParser):
    def error(self, message):
        logging.error(f"Argument parsing error: {message}")
        super().error(message)  # prints the error and raises SystemExit


def main():
    # logging.error(f"sys.argv: {sys.argv}")

    parser = ArgumentParserWithLogging(description="A script to convert speech to text, and save the result in a JSON file.")
    
    # positional arguments
    parser.add_argument('json_file', 			type=str, 	help='Filepath to SpeechToText.json')
    parser.add_argument('start_speech_timeout', type=float, help='Max time to wait for start of speech')
    parser.add_argument('listening_timeout', 	type=float, help='Max time to spend listening to speech')
    parser.add_argument('attempt_id', 			type=str, 	help='Unique ID representing the current speech-to-text attempt')

    # optional arguments
    parser.add_argument('--use-saved-calibration-value', action='store_true', help='Look in SpeechToText.json and use a stored microphone energy threshold value (if it exists)')
    parser.add_argument('--calibrate', action='store_true', help='Flag to calibrate mic for ambient noise instead of listening for speech')

    # parse arguments
    args = parser.parse_args()

    if args.calibrate:
        calibrate_mic(args)
    else:
        transcription = get_transcription(args)

        if transcription:
            with open(args.json_file, 'r+') as file:
                data : dict = json.load(file)
                data.setdefault('transcription', {})['ID'] = args.attempt_id
                data.setdefault('transcription', {})['text'] = transcription
                data.setdefault('transcription', {})['error'] = False
                file.seek(0)        # go to 1st line of file
                json.dump(data, file, indent=4)
                file.truncate()     # remove remaining part
    

def get_transcription(args: argparse.Namespace) -> str | None:
    try:
        recognizer = sr.Recognizer()
        mic = sr.Microphone()

        with open(args.json_file, 'r') as file:
            data : dict = json.load(file)
            if args.use_saved_calibration_value:
                if 'energyThreshold' in data.setdefault('microphoneCalibration', {}):
                    recognizer.energy_threshold = float(data['microphoneCalibration']['energyThreshold'])

        sr.Microphone.list_microphone_names()

        # listen for speech within time limit 
        with mic as source:
            audio = recognizer.listen(source, timeout=args.start_speech_timeout, phrase_time_limit=args.listening_timeout)
        
        transcription = recognizer.recognize_google(audio)

        print(f'transcription: {transcription}')

        return transcription.lower() if transcription else None

    except FileNotFoundError as e:
        handle_error(args, str(e), e)
	# no mic detected
    except OSError:
        handle_error(args, 'No mic detected...')
    # no speech detected
    except sr.WaitTimeoutError:
        default_device_name = pyaudio.PyAudio().get_default_input_device_info()['name']
        handle_error(args, f"No speech detected from '{default_device_name}'. Make sure it's not muted!")
    # API unreachable or unresponsive
    except sr.RequestError:
        handle_error(args, 'Google Speech Recognition API is unavailable')
    # unintelligible speech
    except sr.UnknownValueError:
        handle_error(args, 'Unable to recognize speech')
    # other
    except Exception as e:
        handle_error(args, str(e), e)

    
def calibrate_mic(args: argparse.Namespace):
    try:
        recognizer = sr.Recognizer()
        mic = sr.Microphone()

        print(f'BEFORE: energy_threshold = {recognizer.energy_threshold}')

        with mic as source:
            recognizer.adjust_for_ambient_noise(source)

        print(f'AFTER: energy_threshold = {recognizer.energy_threshold}')

        with open(args.json_file, 'r+') as file:
            data : dict = json.load(file)
            data.setdefault('microphoneCalibration', {})['energyThreshold'] = recognizer.energy_threshold
            data.setdefault('microphoneCalibration', {})['ID'] = args.attempt_id
            file.seek(0)        # go to 1st line
            json.dump(data, file, indent=4)
            file.truncate()     # remove remaining part

    except FileNotFoundError as e:
        handle_error(args, str(e), e)
    except OSError as e:
        handle_error(args, 'No mic detected...')
    except Exception as e:
        handle_error(args, str(e), e)


def handle_error(args, errMsg, e=None):
    logging.error(f"ERROR: {errMsg}")

    if not isinstance(e, FileNotFoundError):
        write_error_to_json(args, errMsg)

    sys.exit(1)
    

def write_error_to_json(args: argparse.Namespace, error_message: str):
    with open(args.json_file, 'r+') as file:
        data : dict = json.load(file)

        if args.calibrate:
            data.setdefault('microphoneCalibration', {})['ID'] = args.attempt_id
            data.setdefault('microphoneCalibration', {})['error'] = True
            data.setdefault('microphoneCalibration', {})['errorMessage'] = error_message
        else:
            data.setdefault('transcription', {})['ID'] = args.attempt_id
            data.setdefault('transcription', {})['error'] = True
            data.setdefault('transcription', {})['errorMessage'] = error_message

        file.seek(0)        # go to 1st line of file
        json.dump(data, file, indent=4)
        file.truncate()     # remove remaining part


if __name__ == '__main__':
    main()
