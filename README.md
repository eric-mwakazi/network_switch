# Network switch simulation

An interactive Layer 2 switch simulation with four virtual NICs. Each NIC has
one connected host. The switch learns source MAC addresses, floods broadcast
and unknown-unicast frames, filters local traffic, and forwards known unicast
frames to one egress NIC.

These NICs exist only inside the program. The simulator does not capture or
inject traffic through Linux network interfaces and does not require root.

## Topology

```text
Host 1 ----- nic1 (port 1) --+
Host 2 ----- nic2 (port 2) --+-- Layer 2 switch
Host 3 ----- nic3 (port 3) --+
Host 4 ----- nic4 (port 4) --+
```

| NIC | Port | Connected host MAC |
| --- | ---: | --- |
| `nic1` | 1 | `00:AA:BB:CC:DD:01` |
| `nic2` | 2 | `00:AA:BB:CC:DD:02` |
| `nic3` | 3 | `00:AA:BB:CC:DD:03` |
| `nic4` | 4 | `00:AA:BB:CC:DD:04` |

## Build and run

```sh
make
./outputs/network_switch
```

Or build and run with one command:

```sh
make run
```

Use `make clean` to remove the compiled executable.

## Betty style checker

The Makefile includes a Betty target for checking all C source and header
files:

```sh
make betty
```

The target expects the `betty` executable to be available in `PATH`. To install
the checker from its upstream repository:

```sh
git clone https://github.com/holbertonschool/Betty.git
cd Betty
sudo ./install.sh
```

Installation writes the `betty`, `betty-style`, and `betty-doc` commands to
`/usr/local/bin`. Review the installation script before running it with
elevated privileges. Back in this project, verify the setup with:

```sh
command -v betty
make betty
```

To use a checker installed under another command or path, override `BETTY`:

```sh
make betty BETTY=/path/to/betty
```

## Commands

### Send a frame

```text
send <source-nic> <destination> <message>
```

The source must be `nic1` through `nic4`. The destination may be another NIC
name, the word `broadcast`, or a raw MAC address. The remaining text becomes
the frame payload.

```text
send nic1 nic2 hello from host 1
send nic2 nic1 reply from host 2
send nic3 broadcast hello everyone
send nic1 00:AA:BB:CC:DD:04 message for host 4
```

### Inspect and exit

```text
nics     Show NIC names, ports, host MAC addresses, and receive counters
table    Show the dynamically learned MAC address table
help     Show command help
quit     Stop the simulation (`exit` also works)
```

## Switching behavior

For each incoming frame, the switch performs these steps:

1. Learn the source MAC address on the ingress port. If that MAC moves, update
	its table entry to the new port.
2. Flood broadcast frames to every NIC except the ingress NIC.
3. Flood unknown unicast frames because the destination port is not learned.
4. Filter a frame when its learned destination is on the ingress port.
5. Forward a known unicast frame only to the learned destination port.

Receiving a flooded frame does not mean that every host accepts it. A host
accepts broadcasts and frames addressed to its MAC; it ignores other unicast
frames. The `received_frames` counter records frames placed on the NIC, even if
the connected host ignores them.

## Example learning flow

```text
switch> send nic1 nic2 first message
[LEARN] Dynamic Entry Added: 00:AA:BB:CC:DD:01 on Port 1
[SWITCH] Unknown destination: flooding all NICs except nic1

switch> send nic2 nic1 reply
[LEARN] Dynamic Entry Added: 00:AA:BB:CC:DD:02 on Port 2
[SWITCH] Known destination: forwarding only to nic1
```

The first frame is flooded because the switch has not learned Host 2 yet. The
reply teaches the switch that Host 2 is on port 2 and uses the already learned
entry for Host 1. Later frames between these hosts can use selective unicast
forwarding.

## Project files

| Path | Purpose |
| --- | --- |
| `main.c` | Frame model, MAC table, switch logic, simulated NICs, and CLI |
| `define.h` | Standard C library includes |
| `Makefile` | Build, run, and clean targets |
| `outputs/network_switch` | Generated executable |

## Limitations

- The NICs and Ethernet frames are in-memory C structures, not Linux devices.
- The simulation does not parse real Ethernet headers or calculate checksums.
- MAC entries do not age out and exist until the program exits.
- There is no VLAN, spanning tree, link state, bandwidth, or timing model.
- Input MAC addresses are treated as labels and are not syntax-validated.

## Extending the simulation

To add ports, update `NIC_COUNT` and add entries to the `nics` array in
`main.c`. Each entry needs a unique NIC name, host MAC address, and port number:

```c
{"nic5", "00:AA:BB:CC:DD:05", 5, 0}
```
