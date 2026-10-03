"""Audit the vendor Makefile profiles, which are not represented in .config."""
import re
from pathlib import Path

profiles = {
    'techpack/display/config/lahainadisp.conf': 'CONFIG_DISPLAY_BUILD',
    'techpack/display/config/gki_lahainadisp.conf': 'CONFIG_DISPLAY_BUILD',
    'techpack/audio/config/lahainaauto.conf': 'CONFIG_SND_SOC_LAHAINA',
    'techpack/camera/config/lahainacamera.conf': 'CONFIG_SPECTRA_CAMERA',
    'techpack/camera/config/yupikcamera.conf': 'CONFIG_SPECTRA_CAMERA',
}
for filename, required in profiles.items():
    text = Path(filename).read_text(encoding='utf-8')
    modules = re.findall(r'^(?:export )?(CONFIG_\w+)=m\b', text, re.MULTILINE)
    if modules:
        raise SystemExit(f'{filename}: unexpected modular vendor drivers: {modules}')
    if not re.search(rf'^(?:export )?{required}=y$', text, re.MULTILINE):
        raise SystemExit(f'{filename}: {required} must be built-in')
    print(f'{filename}: built-in vendor profile verified')
