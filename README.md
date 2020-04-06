# moisture

## Requirements

[ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/) installed.

## Building

### Getting mqtt server certificate

```
openssl s_client -showcerts -connect SERVER:PORT </dev/null 2>/dev/null | openssl x509 -outform PEM > main/certificate.pem
curl "https://letsencrypt.org/certs/letsencryptauthorityx3.pem.txt" >> main/certificate.pem
```

### Configure project

Configure the properties in the _Moisture_ section of the config menu.
```
idf.py menuconfig
```

### Compile

```
cd moisture
idf.py build
```
