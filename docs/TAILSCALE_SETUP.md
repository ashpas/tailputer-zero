# One-time setup Tailputer Zero depends on

Tailputer Zero never runs as root (the CardputerZero App Store rejects any
package that does). It drives an already-installed, already-running
Tailscale daemon through one fixed-verb helper script authorized via a
narrowly-scoped sudoers rule. Do this once over SSH before installing the
app itself.

## One-shot setup (recommended)

```sh
scp -r scripts/ <user>@<device-ip>:/tmp/tailputer-zero-setup
ssh <user>@<device-ip>
sudo /tmp/tailputer-zero-setup/setup-tailputer-zero.sh
```

This installs Tailscale if it's missing, runs `tailscale up` if it isn't
already connected (interactive login prompt the first time), installs the
helper script, and generates + validates + installs the sudoers rule for
whichever user you ran `sudo` as -- no manual `<app-user>` editing needed.
It's idempotent, so re-running it later (e.g. after a Tailscale reinstall)
is safe.

Once it finishes, install Tailputer Zero's `.deb` -- Connect/Disconnect
will work immediately.

## What the script does (manual steps, for review or if you'd rather not pipe a script into sudo)

### 1. Install Tailscale

```sh
curl -fsSL https://tailscale.com/install.sh | sh
sudo tailscale up   # complete the interactive login this first time
```

### 2. Install the helper script

```sh
sudo install -m 0755 scripts/cardputerzero-tailscale-helper.sh \
  /usr/local/bin/cardputerzero-tailscale-helper
```

### 3. Authorize it via sudoers

Edit `scripts/99-cardputerzero-tailscale`, replacing `<app-user>` with the
account the app actually runs as (check with `whoami` over SSH, or `id`
while the app is running). Then:

```sh
sudo visudo -c -f scripts/99-cardputerzero-tailscale   # validate first
sudo install -m 0440 -o root -g root \
  scripts/99-cardputerzero-tailscale /etc/sudoers.d/99-cardputerzero-tailscale
```

This grants `NOPASSWD` only on the four exact helper invocations
(`status`, `up`, `up-with-key <key>`, `down`) -- never on the real
`tailscale` binary, and never with an unconstrained wildcard. The one-shot
script above generates this exact same rule automatically.

## Known limitations (honest, not yet hardened)

- `up-with-key` passes the auth key as a single argv token to the helper
  script. It never reaches a shell, so there's no injection surface, but it
  is visible to other local users via `/proc/<pid>/cmdline` for the
  (sub-second) life of that process. Piping the key over stdin instead is a
  documented follow-up, not yet implemented.
- Canceling the QR login screen does not kill the in-flight `tailscale up`
  process; it just stops showing it. The process finishes or times out on
  its own.
- Built with the `cp0-cross` preset and installed/tested on a real
  CardputerZero across multiple releases; every Connect/Disconnect, QR
  login, log viewer, and quit-confirmation bug fixed so far was found
  through actual on-device testing, not just the desktop simulator. Per the
  CardputerZero App Submission Guide, a store submission still needs a
  physical-device demo video -- that part doesn't exist yet.
