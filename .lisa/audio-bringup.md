# Lisa audio bring-up

The user reports that calls establish on the `4083eb08` candidate but have
no sound. Read-only diagnostics show no ALSA sound cards and no PCM devices.
The machine driver remains deferred at PRI_MI2S_RX. The ADSP and WCD938x
codec initialize; this is not sufficient to register a usable sound card.

Two integration gaps are present:

- The build omitted TARGET_PRODUCT=lisa. The vendor Kbuild consequently
  skipped the existing TFA98xx amplifier directory and Lisa TDM machine
  definitions. The live device tree has two enabled `tfa,tfa98xx` devices
  at I2C addresses 0x34 and 0x35. The installed Image has no TFA probe symbol.
- The newer MI2S driver requires `qcom,msm-mi2s-lines` and separate direction
  device IDs. Lisa's live device tree has combined RX/TX interface IDs and
  separate `qcom,msm-mi2s-rx-lines` / `qcom,msm-mi2s-tx-lines` properties.
  MI2S platform devices exist but have no bound driver/components.

The Lisa build now supplies the real target product. Its combined MI2S path
uses the original GPL-2.0-only implementation from MiCode lisa-r-oss commit
`57ee2b068000ac79caa7c076d0d9bb1a9260e8a4`. The code is isolated in
`msm-dai-q6-mi2s-lisa.h`, selected only by CONFIG_TARGET_PRODUCT_LISA. Other
targets retain the newer separate-direction implementation. RX/TX line
masks, stream configuration and errors use actual device-tree values.

Vendor preflight and final Image verification require the TFA probe provider.
The existing device tree, ROM/vendor files and firmware remain untouched.
Full CI, sound-card registration, media playback, call audio and microphone
tests are still required before claiming audio works. Private device records
are not committed.
