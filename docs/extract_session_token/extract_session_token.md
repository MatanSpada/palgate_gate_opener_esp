<p align="center">
  <img src="../logo.png" alt="PalGate Logo" width="160">
</p>

<h3 align="center">Extracting Your Permanent Session Token</h3>

<p align="center">
  A clean and safe way to generate your PalGate session token<br>
  using the official linking mechanism.
</p>

---

## Table of contents
- [Overview](#overview)
- [Requirements](#requirements)
- [Step 1 - Install pyenv](#step-1---install-pyenv)
- [Step 2 - Install Python 3.11.6](#step-2---install-python-3116)
- [Step 3 - Create a virtual environment](#step-3---create-a-virtual-environment)
- [Step 4 - Install pylgate](#step-4---install-pylgate)
- [Step 5 - Install dependencies](#step-5---install-dependencies)
- [Step 6 - Generate the QR code](#step-6---generate-the-qr-code) 
- [Step 7 - Scan using the PalGate app](#step-7---scan-using-the-palgate-app)

---

## Overview
PalGate’s linking flow allows adding “virtual devices” by scanning a QR code.  
The `pylgate` tool acts as a new virtual device.  
When you scan the QR code using your PalGate app,  
PalGate generates a **permanent session token** for that virtual device.

This token is what we use next.

This guide walks you through:
- Installing a clean Python environment (needed only if your Python version is lower then 3.9)
- Installing pylgate
- Running the linking script
- Extracting the session token

---

## Requirements
Before installing pyenv or Python:

```bash
sudo apt update
sudo apt install -y build-essential curl git \
    libssl-dev zlib1g-dev libbz2-dev libreadline-dev \
    libsqlite3-dev libffi-dev liblzma-dev
```
These are required for building Python via pyenv.
If your system already has Python 3.9 or higher installed, you can skip directly to [Step 4 - Install pylgate](#step-4---install-pylgate) inside a virtual environment.

<br><br>


# Steps

## Step 1 - Install pyenv
```bash
curl https://pyenv.run | bash
```
Add pyenv initialization to your shell (here is a ZSH example):
```bash
  echo '' >> ~/.zshrc
  echo '# >>> pyenv initialization >>>' >> ~/.zshrc
  echo 'export PYENV_ROOT="$HOME/.pyenv"' >> ~/.zshrc
  echo 'export PATH="$PYENV_ROOT/bin:$PATH"' >> ~/.zshrc
  echo 'eval "$(pyenv init - zsh)"' >> ~/.zshrc
  echo 'eval "$(pyenv virtualenv-init -)"' >> ~/.zshrc
  echo '# <<< pyenv initialization <<<' >> ~/.zshrc
  source ~/.zshrc
```
Verify installation:
```bash
pyenv --version
```

## Step 2 - Install Python 3.11.6
```bash
pyenv install 3.11.6
```

## Step 3 - Create a virtual environment
```bash
pyenv virtualenv 3.11.6 pylgate-env
pyenv activate pylgate-env
python --version
```

Expected:
```bash
Python 3.11.6
```

## Step 4 - Install pylgate
Because pip may point to the system Python, use full path:

```bash
~/.pyenv/versions/pylgate-env/bin/pip install \
    git+https://github.com/DonutByte/pylgate.git@main
```

## Step 5 - Install dependencies
Install QR + PIL:

```bash
~/.pyenv/versions/pylgate-env/bin/pip install "qrcode[pil]"
```

Install Requests:
```bash
~/.pyenv/versions/pylgate-env/bin/pip install requests
```

## Step 6 - Generate the QR code
run:

```bash
python generate_token.py
```

This will:

- Generate a QR code
- Wait for you to scan
- Complete the linking flow
- Output the permanent session token

## Step 7 - Scan using the PalGate app

On your PalGate mobile app:

Menu → Device Linking → Link a Device → Scan QR
After scanning, the terminal will show something like:

```bash
checking status...
updating user info...
checking derived token...
Logged-in successfully :)
Phone number (user id): 9725XXXXXXXX
Session token: eb70ce644902149853426ddf07da092d
Token type: 1 (TokenType.PRIMARY)
```

Your session token is now ready to use in the main project.
Copy the `Session token` value into your `config.h` as `SESSION_TOKEN_HEX`.
---
