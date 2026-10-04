# Lisa vibration bring-up

On the verified Wi-Fi candidate `de5d98fd`, AW8624 probe returns -EBUSY at
its IRQ GPIO request. The live device tree assigns TLMM GPIO 18 to both
`awinic,aw8624_haptic` irq-gpio and the generic RTC6226 FM node fmint-gpio.
The FM driver probes first and requests that GPIO before the AW8624 driver.
The haptic I2C device consequently has no bound driver or vibrator endpoint.

The Lisa recipe excludes the mutually conflicting optional RTC6226 radio
backend by disabling I2C_RTC6226_QCA. It keeps INPUT_AW8624_HAPTIC built-in
and explicitly verifies both selections after olddefconfig. No GPIO ownership
bypass, driver success emulation, live unbind, or device-tree replacement is
used. Other kernel build recipes retain their original radio configuration.

FM receiver support is excluded from this Lisa candidate. The resulting
Image must be verified, followed by actual AW8624 probe/firmware loading and
user vibration testing; configuration alone is not proof of working haptics.
