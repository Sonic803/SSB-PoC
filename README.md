# Proof of concept e mitigazione della vulnerabilità Speculative Store Bypass nel nucleo didattico

Dispensa: <https://calcolatori.iet.unipi.it/resources/nucleo.pdf>

## Requisiti

- [Libce](https://calcolatori.iet.unipi.it/istruzioni_libce.php)
- [QEMU](https://calcolatori.iet.unipi.it/istruzioni_qemu.php)
- Processore Intel Coffee Lake o successivo

## Esecuzione

```bash
make &&
./run -enable-kvm -cpu host
```
