# Program simulating a ticket machine and its server
## Introduction:
I was originally requested to write the program in question as a part of a recruitation process. Back then I failed to finish it in time, and I was unhappy with what I did write.
The current iteration of the program is what I managed to accomplish without time pressure. It's not completely finished yet; as I'm actively using it to practice my knowledge, I plan to expand it further, but in its current form it already meets the parameters of the original task.

## Description:
The program is simulating a ticket machine accepting cash and printing tickets. In case of too large amount inserted, it gives away change in possibly smallest amount of coins/bills.
The program does not work without a server, which in turn accepts or rejects the transactions, and decides what tickets are avalible.
Inserting money into the machine is simulated by typing in the value in cents. I picked dollars as a currency because the program is written in English language.

## Compilation
Make sure you have Qt and CMake installed

### Linux
Enter the following commands sequentially into the console in the root directory:

```
cmake -S . -B build
cmake --build build --config Release
```
You will find the executables in the build directory.

### Windows 
Enter the following commands sequentially into the console in the root directory, <br>
<b>Replace the path in the first command with the location of your Qt installation</b>
```
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/qt/6.8.3/msvc2022_64
cmake --build build --config Release
cmake --install build --config Release
```
You will find the executables in the dist/bin directory.

## Completed features
- UI
- Client-Server communication
- Functional transaction, both client-side and server-side
- Logging system; logging sensitive data is protected behind a macro that can't be defined in the release builds
- The server pre-reserves a single instance of a ticket, so that other users can't buy it when the original user is mid-transaction.
- The client correctly calculates and gives out change.

## Planned features
- Pre-reservation timeout, when a user spends too much time on the transaction.
- Connecting the server to a database so that purchases can be registered.
- Connecting client(s) to databases so they can register how much money is inside.
- Moving/option of moving logs from the console to dedicated log files.
- Localization of the program to more languages (Though I'm probably too lazy for this one)

## Disclaimer
As this program is meant to serve as a proof of my programming skill, that I know and understand topics used in the code, it was obviously written by me; that is, it was not written by AI.
That being said, I did use AI to find typos and bugs in the code, and as a tool for finding relevant parts in the documentation. Besides that, CMakeLists.txt was written while heavily relying on AI. I wish I knew how to write it myself, but it's something done so rarely, it doesn't tend to have a staying power in my head.