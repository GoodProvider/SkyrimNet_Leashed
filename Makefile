VERSION=0.3.0
NAME=SkyrimNet Leashed

RELEASE_FILE=versions/SkyrimNet_Leashed ${VERSION}.7z

.PHONY: esp release

# Rebuild ESP from Spriggit/ (source of truth).
esp:
	powershell -NoProfile -File python_scripts/deserialize_esp.ps1 -RepoRoot "$(CURDIR)"

# Stamp FOMOD, deserialize ESP, pack versions/*.7z (no PDBs).
release:
	python python_scripts/fomod-update-name-version.py -v ${VERSION} -n "${NAME}" -o FOMOD/info.xml FOMOD_source/info.xml
	$(MAKE) esp
	powershell -NoProfile -File python_scripts/pack_release.ps1 -Version ${VERSION} -Name "${NAME}" -RepoRoot "$(CURDIR)"
