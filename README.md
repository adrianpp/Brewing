# Brewing

A brewing controller based on a raspberry pi, custom pi hat, and crow library for web interface.

# Setting up the pi

Clone the repo using ssh. see below links for setting up ssh:

https://docs.github.com/en/authentication/connecting-to-github-with-ssh/using-ssh-agent-forwarding

https://docs.github.com/en/authentication/connecting-to-github-with-ssh/working-with-ssh-key-passphrases

after cloning, cd into the cloned directory and `make`.

can use `make mock` to not use wiringPi and instead use mock interface.

# Hardware Connections

## 1-wire/i2c:

| Aviation Pin  | Connection  | Color |
|---------------|-------------|-------|
| 1             | ground      | black |
| 2             | power       | red   |
| 3             | signal      | green |

## 3 way valve (cr02 wiring):

| Aviation Pin  | Connection  | Color |
|---------------|-------------|-------|
| 1             | close       | blue  |
| 2             | open        | brown/red   |
| 3             | ground      | yellow |
