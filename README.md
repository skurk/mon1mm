# mon1mm

MoN1MM ("monitor" and "n1mm" combined, assuming N1MM is prononunced as rhyming with 'limb') is a small
Linux console application (C11) that listens for [N1MM Logger+](https://n1mmwp.hamdocs.com/)
UDP broadcasts and stores the contact data in a MySQL/MariaDB database.

It handles the `contactinfo`, `contactreplace`, and `contactdelete` message
types and ignores everything else (score, radio, spots, etc.).

The purpose of this application is to, at some point, automatically and
periodically sync new contacts with LoTW.

## Features

- UDP listener bound to `0.0.0.0` on a configurable port (default **12060**).
- XML parsing with **libexpat**.
- MySQL persistence using the **official MySQL C API** with **prepared
  statements** and bound parameters (no string concatenation).
- Upsert (`INSERT ... ON DUPLICATE KEY UPDATE`) keyed on the `ID` element.
- Automatic DB reconnect with exponential backoff.
- INI-style configuration file with sensible defaults.
- Timestamped logging and graceful `SIGINT`/`SIGTERM` shutdown.
- Never crashes on malformed XML — it logs and continues.

## Configure

Copy and edit the sample config:

```sh
cp mon1mm.conf /etc/mon1mm.conf   # or keep it next to the binary
```

Keys (with defaults):

| Key           | Default     | Description                     |
|---------------|-------------|---------------------------------|
| `udp_port`    | `12060`     | UDP port to listen on           |
| `db_host`     | `localhost` | MySQL host                      |
| `db_port`     | `3306`      | MySQL port                      |
| `db_user`     | `root`      | MySQL user                      |
| `db_password` | *(empty)*   | MySQL password                  |
| `db_name`     | `n1mm`      | Database name                   |

## Run

```sh
./mon1mm [path/to/config]
```

If no path is given it uses `./mon1mm .conf`. Stop it with `Ctrl+C`
(`SIGINT`) or `SIGTERM`; it closes the socket and DB connection cleanly.

## Configure N1MM to broadcast

In N1MM Logger+:

1. Open **Config → Configure Ports, Mode Control, Audio, Other → Broadcast
   Data** tab.
2. Enable **Contacts** (and Contact Delete/Replace as desired).
3. Set the **IP:port** destination to this host's IP and the configured port,
   e.g. `192.168.1.50:12060`. For a single destination use the unicast IP; you
   can also use a subnet broadcast address such as `192.168.1.255:12060`.
4. Apply/OK. New QSOs will now be sent as XML datagrams and stored in MySQL.

Make sure any firewall on this host allows inbound UDP on the chosen port:

```sh
sudo ufw allow 12060/udp
```

## Project layout

| File               | Purpose                                     |
|--------------------|---------------------------------------------|
| `main.c`           | Entry point, event loop, signal handling    |
| `config.c/.h`      | INI configuration parser                    |
| `udp.c/.h`         | UDP socket setup                            |
| `xmlparse.c/.h`    | libexpat parsing into a contact record      |
| `db.c/.h`          | MySQL connect, upsert, delete, reconnect    |
| `log.c/.h`         | Timestamped logging helpers                 |
| `schema.sql`       | Database + `contacts` table schema          |
| `mon1mm.conf`      | Sample configuration                        |
| `Makefile`         | Build script                                |


## Work in progress

This is a project under development. Stability and functionality may be so-so.
