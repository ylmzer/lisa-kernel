from pathlib import Path
import hashlib, zipfile
from datetime import datetime, timezone, timedelta
root=Path('out/anykernel');dist=Path('dist/lisa')
stamp=datetime.now(timezone(timedelta(hours=3))).strftime('%d-%m-%Y-%H-%M')
package=dist/f'ylmzer-HyperOSPro-IORAP-TEST-{stamp}.zip'
with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(root.rglob('*')):
  rel=p.relative_to(root)
  if p.is_file() and not any(x.startswith('.') for x in rel.parts) and p.name!='README.md' and 'placeholder' not in p.name:
   z.write(p,rel.as_posix())
with zipfile.ZipFile(package) as z:
 assert z.testzip() is None
 assert hashlib.sha256(z.read('Image')).digest()==hashlib.sha256((dist/'Image').read_bytes()).digest()
names=['Image',package.name,'lisa-iorap-test-arm64','resolved.config','System.map']
(dist/'SHA256SUMS-TEST').write_text(''.join(hashlib.sha256((dist/n).read_bytes()).hexdigest()+'  '+n+'\n' for n in names))
print(package)
