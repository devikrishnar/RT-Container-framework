# RT-Container-framework
A framework help to manage Real-Time Container in a distributed IoT system.
This framework provides following

## Before you start
This framework includes the monitor and migration executor for container migration, thus yoou need to prepare multiple devices as migration target before you install it.
## Installation
The default installation can be through simple command:
```sh
make
```

Some other configuration options are:
### RT-controller setup only
```sh
make rt-controller
```
### Monitoring framework setup only
```sh
make monitor
```
### Migration decision rules
We can accept customized migration decision rules through `rules` file.
Then use the following command to install our framework by your own rules:
```sh
make custom-rule
```
## Testing
