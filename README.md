# Phone Exchange #

## Goal whit the project ##
This project aims to create a local wired phone network that supports both old rotary phones and newer DTMF tone-dialing phones. No intelligence or electronics will be built into the phones themselves; all logic and switching will be handled by the exchange unit. The network will follow a star topology, with two wires leading to each phone.

The goal is for the network to behave just like it did back in the day. Features such as ringing behavior, tones, and call logic will be implemented.

This is a learning project and my personal introduction to programming, Arduino, HTML, and electronics. It might eventually result in nothing more than a pile of electronic scrap and hundreds of non-functional lines of code—but it will definitely provide me with valuable knowledge and a steep learning curve.





### Typical behaviour ###

A typical procedure of the system will be as shown below.
- A hook is lifted, and the status of the line changes from "idle" to "ready."
- A phone number is dialed using pulse dialing or DTMF tones, and the digits are captured by the microcontroller.
- Ring pulses are generated for the dialed line. Once the hook is picked up, the audio lines are connected between the two lines and the conversation can begin
- Ones one of the connected phones puts it's handset into the crank, the audio connection between the lines breaks and the call ends

#### Line Statuses ####

To handle the logics in the exchange system, all lines has a current status whish triggers different logics in the system when they are changed.

- Hook  :telephone_receiver: - If the phone hook is on or off due to the status
- Timer :stopwatch: - If the new status initiate a timer
- Audio :loud_sound: -  If a audio is played to the line due to the status (might not be implemented)
- Tone :postal_horn: - if a tone is played to the line due to the status

| Phone status  | Function                              | Hook     | Timer | Audio | Tone |
| ------------- | ------------------------------------- | -------- | ----- |-------|------| 
| idle          | Line is not in use                    | ✅ ON    | ❌    | ➖    | ❌   |
| ready         | Line is ready, waiting for inputs     | ⚪ OFF   | ✅    | ➖    | ✅   |
| puls_dialing  | Rotary dialing in progress            | ⚪ OFF   | ✅    | ➖    | ❌   |
| tone_dialing  | Touch-tone dialing in progress        | ⚪ OFF   | ✅    | ➖    | ❌   |
| busy          | Receiving busy signal                 | ⚪ OFF   | ✅    | ➖    | ✅   |
| fail          | Line failed to connect                | ⚪ OFF   | ✅    | ➖    | ✅   |
| ringing       | Line is ringing (outgoing call)       | ⚪ OFF   | ✅    | ➖    | ✅   |
| connected     | Call is active                        | ⚪ OFF   | ❌    | ➖    | ❌   |
| disconnected  | Call has ended, but line not yet idle | ⚪ OFF   | ✅    | ➖    | ❌   |
| timeout       | Line timed out                        | ⚪ OFF   | ✅    | ➖    | ✅   |
| abandoned     | Line was abandoned                    | ⚪ OFF   | ✅    | ➖    | ✅   |
| incoming      | Incoming call                         | ✅ ON    | ❌    | ➖    | ❌   |
| operator      | Connected to operator                 | ➖       | ❌    | ➖    | ❌   |
