✅ Guide: Extracting a Permanent PalGate Session Token

This guide explains exactly how to extract the permanent PalGate session token using the pylgate tool, in a safe and isolated environment.

⭐ Overview

PalGate allows linking additional devices using a “Device Linking” flow (QR code scanning).
The pylgate tool pretends to be an additional device, and when the QR code is scanned from your PalGate mobile app, the server generates a permanent session token for that virtual device.

This token is what we need for this project.

This guide shows how to:

✔ Install a safe Python environment using pyenv if your Python version is lower
✔ Install pylgate
✔ Install dependencies (qrcode, pillow, requests)
✔ Run the linking script
✔ Extract the permanent session token

The pylgate package requires Python ≥ 3.9.

If your system already has Python 3.9 or higher installed, you can skip the entire pyenv installation process and go directly to Step 4 - Install pylgate inside a virtual environment.

📦 Requirements

Before you begin, install the required build dependencies:

sudo apt update
sudo apt install -y build-essential curl git \
    libssl-dev zlib1g-dev libbz2-dev libreadline-dev \
    libsqlite3-dev libffi-dev liblzma-dev


These are mandatory for building Python via pyenv.


🧰 Step 1 — Install pyenv (Safe Python Version Manager)
curl https://pyenv.run | bash


Add pyenv initialization to your ~/.zshrc (for zsh users):

echo '' >> ~/.zshrc
echo '# >>> pyenv initialization >>>' >> ~/.zshrc
echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.zshrc
echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.zshrc
echo 'eval "$(pyenv init - zsh)"' >> ~/.zshrc
echo 'eval "$(pyenv virtualenv-init -)"' >> ~/.zshrc
echo '# <<< pyenv initialization <<<' >> ~/.zshrc

source ~/.zshrc


Verify installation:

pyenv --version



🐍 Step 2 — Install a clean Python version for pylgate

Install Python 3.11.6:

pyenv install 3.11.6


(This may show a harmless warning about _tkinter. Ignore it.)



🧪 Step 3 — Create an isolated virtual environment
pyenv virtualenv 3.11.6 pylgate-env
pyenv activate pylgate-env


Check the Python version:

python --version


You must see:

Python 3.11.6




📦 Step 4 — Install pylgate 

If you are using zsh, it may still use /usr/bin/pip.
To avoid this, always call pip by full path:

~/.pyenv/versions/pylgate-env/bin/pip install git+https://github.com/DonutByte/pylgate.git@main


If installation succeeds, proceed.



📚 Step 5 — Install dependencies (required for the QR script)
Install QR code generation + image backend:
~/.pyenv/versions/pylgate-env/bin/pip install "qrcode[pil]"

Install requests:
~/.pyenv/versions/pylgate-env/bin/pip install requests



🚀 Step 6 — Run the script to generate the QR code
python generate_token.py


The script will:

Generate a QR code

Wait for a scan

Perform the PalGate linking flow

Output the “permanent session token”



📱 Step 7 — Scan QR code from PalGate mobile app

On your PalGate mobile app:

Menu → Device Linking → Link a Device → Scan QR


After scanning, your terminal will display something like:

checking status...
updating user info...
checking derived token...
Logged-in successfully :)
Phone number (user id): 9725XXXXXXXX
Session token: eb70ce644902149853426ddf07da092d
Token type: 1 (TokenType.PRIMARY)


