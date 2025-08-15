# @emolitor's AKA emulator's QMK Community Modules

These rely on QMK Firmware 0.28.0 or later, merged in 2025q1.

In order to use these community modules with a build of QMK, this repo should be added to your external userspace as a submodule.

```sh
cd /path/to/your/external/userspace
git submodule add https://github.com/emolitor/qmk_modules.git modules/emolitor
git submodule update --init --recursive
```

Each child directory is a separate module, and has instructions on how to add it to your build.

| Module | Description |
| ------ | ----------- |

These modules are under development and may not be working properly yet.

| Module | Status | Description |
| ------ | ------ | ----------- |
| [Westberry Wireless](./westberry_wireless)| In Development | QMK support for Westberry Technology's CH582F wireless submodule |
