#!/usr/bin/env python3
"""Fail closed on deviations from the device-validated native build profile."""
from pathlib import Path
import hashlib,json,subprocess,sys
root=Path(__file__).resolve().parent.parent
profile=root/'.lisa/profiles/native-20261023'
m=json.loads((profile/'manifest.json').read_text())
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
def check(ok,why):
 if not ok:raise SystemExit('PROFILE REJECTED: '+why)
for n,h in m['files'].items():check(sha(profile/n)==h,'reference hash '+n)
mode=sys.argv[1] if len(sys.argv)>1 else 'environment'
check(mode in ['environment','config','artifacts'],'unknown validation mode')
if mode=='environment':
 osinfo=dict(line.split('=',1) for line in Path('/etc/os-release').read_text().splitlines() if '=' in line)
 check(osinfo['VERSION_ID'].strip('"')==m['ubuntu_version'],'Ubuntu version')
 arch=subprocess.check_output(['dpkg','--print-architecture'],text=True).strip()
 check(arch==m['host_arch'],'host architecture')
 for n,v in m['packages'].items():
  actual=subprocess.check_output(['dpkg-query','-W','-f=${Version}',n],text=True)
  check(actual==v,'package '+n+' expected '+v+' got '+actual)
 for n,t in m['tools'].items():check(sha(Path(t['path']))==t['sha256'],'compiler/tool '+n)
 for n,h in m['toolchain_libraries'].items():check(sha(Path(n))==h,'toolchain library '+n)
else:
 dist=root/'dist/lisa'
 check((dist/'resolved.config').read_bytes()==(profile/'resolved.config').read_bytes(),'resolved config differs')
 if mode=='artifacts':
  def symbols(p):return {v[1]:(v[0],v[2:]) for line in p.read_text().splitlines() if (v:=line.split())}
  check(symbols(dist/'Module.symvers')==symbols(profile/'Module.symvers'),'module symbol/CRC mismatch')
  sm=(dist/'System.map').read_text()
  check('lisa_iorap_test_init' not in sm,'diagnostic endpoint enabled')
  for n in ['legacy_ioctl','lisa_iorap_anon_willneed']:check(n in sm,'missing symbol '+n)
  info=dict(line.split('=',1) for line in (dist/'build-info.txt').read_text().splitlines() if '=' in line)
  check(info['mode']=='build','artifact verification requires a full build, not stale outputs')
  check(info['build_timestamp']==m['build_timestamp'],'timestamp differs')
  for line in (dist/'SHA256SUMS').read_text().splitlines():
   h,n=line.split(None,1);check(sha(dist/n.strip())==h,'artifact manifest '+n.strip())
  report={'profile':m['profile'],'source_commit':info['source_commit'],'config_exact':True,'all_module_crc_equal':True,'required_symbols_present':True,'diagnostic_absent':True,'image_sha256':sha(dist/'Image'),'reference_image_sha256':m['reference_image_sha256'],'binary_identical_to_reference':sha(dist/'Image')==m['reference_image_sha256'],'new_build_device_tested':False}
  (dist/'validated-profile.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS validated native profile: '+mode)
