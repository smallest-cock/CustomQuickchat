#! /usr/bin/env pythonw

import speech_recognition as sr
import time
import json
import sys


args = sys.argv[1:]


def main():

    if not args:
        print("Error: too few arguments provided")
        return

    attemptID = args[2]

    transcription = get_transcription()
    current_time = time.time()

    if transcription:
        print(transcription)
        # update JSON
        with open(args[0], 'r+') as f:      # json filepath should be the 1st argument
            data = json.load(f)
            data['transcription']['ID'] = attemptID
            data['transcription']['time'] = current_time 
            data['transcription']['text'] = transcription
            data['transcription']['error'] = False
            f.seek(0)        # <--- should reset file position to the beginning.
            json.dump(data, f, indent=4)
            f.truncate()     # remove remaining part
    

def get_transcription() -> str | None:
    try:
        recognizer = sr.Recognizer()
        mic = sr.Microphone()

        timeoutArg = args[1]

        with mic as source:
            audio = recognizer.listen(source, timeout=float(timeoutArg))    # listen within time limit for the start of speech

        transcription = recognizer.recognize_google(audio)
        return transcription.lower() if transcription else None

    except OSError:
        write_error_to_json(args[0], 'No mic detected...')
    except sr.WaitTimeoutError:
        write_error_to_json(args[0], "You didn't speak or your mic is muted")
    except sr.RequestError:
        # API was unreachable or unresponsive
        write_error_to_json(args[0], 'Google Speech Recognition API is unavailable')
    except sr.UnknownValueError:
        # speech was unintelligible
        write_error_to_json(args[0], 'Unable to recognize speech')
    except Exception as e:
        # print(e)
        write_error_to_json(args[0], str(e))

    
def write_error_to_json(filepath: str, error_message: str):
    attemptID = args[2]

    with open(filepath, 'r+') as f:      # json filepath should be the 1st argument
        data = json.load(f)
        data['transcription']['ID'] = attemptID
        data['transcription']['error'] = True
        data['transcription']['errorMessage'] = error_message
        f.seek(0)        # <--- should reset file position to the beginning.
        json.dump(data, f, indent=4)
        f.truncate()     # remove remaining part


if __name__ == '__main__':
    main()
