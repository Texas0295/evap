# evap

A minimal ephemeral editing buffer for the terminal.

It launches your preferred editor to enter a short message or note.
After quit, the content is printed to stdout and the temporary file is deleted.

## Example

```sh
evap | grep texas
````

## Installation

```sh
make
sudo make install
```

## License

MIT License. See [LICENSE](./LICENSE)

## More

For detailed usage:

```sh
man evap
```
