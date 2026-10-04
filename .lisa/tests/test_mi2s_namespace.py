"""Check the actual machine header against combined/separate DAI contracts."""
from pathlib import Path
import re, subprocess, tempfile

header = Path('techpack/audio/asoc/msm_dailink.h').resolve()
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    (root / 'sound').mkdir()
    (root / 'sound/soc.h').write_text('')
    for lisa in (False, True):
        args = ['gcc', '-E', '-P', '-x', 'c', '-I', str(root)]
        if lisa:
            args.append('-DCONFIG_TARGET_PRODUCT_LISA')
        output = subprocess.check_output(args + [str(header)], text=True)
        links = {}
        pattern = (r'SND_SOC_DAILINK_DEFS\(((?:pri|sec|tert|quat|quin|sen)_mi2s_(?:rx|tx)),'
                   r'\s*DAILINK_COMP_ARRAY\(COMP_CPU\((.*?)\)\)')
        for name, expression in re.findall(pattern, output, re.S):
            links[name] = ''.join(re.findall(r'"([^"\n]*)"', expression))
        assert len(links) == 12, links
        if lisa:
            # Six full-duplex interfaces, matching the live Lisa DT aliases.
            assert set(links.values()) == {f'msm-dai-q6-mi2s.{i}' for i in range(6)}
            for prefix in ['pri', 'sec', 'tert', 'quat', 'quin', 'sen']:
                assert links[prefix + '_mi2s_rx'] == links[prefix + '_mi2s_tx']
        else:
            # Keep all 12 separate-direction DAIs on newer device trees.
            assert set(links.values()) == {f'msm-dai-q6-mi2s.{i}' for i in range(12)}
print('Actual MI2S machine header matches Lisa full-duplex and native separate-direction namespaces')
