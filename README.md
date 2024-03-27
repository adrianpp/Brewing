# Brewing

A brewing controller based on a raspberry pi, custom pi hat, and crow library for web interface.

# Setting up the pi

Clone the repo using ssh. see below links for setting up ssh:

https://docs.github.com/en/authentication/connecting-to-github-with-ssh/using-ssh-agent-forwarding

https://docs.github.com/en/authentication/connecting-to-github-with-ssh/working-with-ssh-key-passphrases

after cloning, cd into the cloned directory and `make`.

can use `make mock all` to not use wiringPi and instead use mock interface.

Will almost certainly need to autostart the web service; can add a line to `crontab -e` like:

`@reboot sleep 60; cd /home/admin/Brewing && ./build/apps/run_brewery >> serverexec.log`

Also be sure to set up gpio by adding a line to `/boot/config.txt` like:

`dtoverlay=gpio`

# Brewery Plumbing Design

![3 vessel plumbing](https://github.com/adrianpp/Brewing/blob/master/docs/hardplumb_3_vessel.png?raw=true)

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

## Hall-effect Flow Meter:

| Aviation Pin  | Connection  | Color |
|---------------|-------------|-------|
| 1             | ground       | black  |
| 2             | power        | red   |
| 3             | signal       | yellow |

## Heater:

| TRS Jack  | Connection  | Color |
|---------------|-------------|-------|
| Sleeve        | ground       | black  |
| Ring          | power        | red   |
| Tip           | signal       | yellow |

## Breakout Board
![schematic](https://github.com/adrianpp/Brewing/blob/master/docs/Schematic_Beer%20Breakout%20with%20Relays_2024-03-19.png?raw=true)
![top layer](https://github.com/adrianpp/Brewing/blob/master/docs/TOP_Beer%20Breakout%20with%20Relays_2024-03-19.png?raw=true)
![bottom layer](https://github.com/adrianpp/Brewing/blob/master/docs/BOT_Beer%20Breakout%20with%20Relays_2024-03-19.png?raw=true)
