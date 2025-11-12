# TouchAxe - Déploiement GitHub Pages

## 🚀 Guide de Déploiement Rapide

### Option 1 : Utiliser le dossier `docs/` (RECOMMANDÉ)

1. **Créer le dossier docs**
   ```bash
   mkdir docs
   cp -r webflasher/* docs/
   ```

2. **Commit et push**
   ```bash
   git add docs/
   git commit -m "Add web flasher to docs"
   git push origin main
   ```

3. **Activer GitHub Pages**
   - Aller sur votre repo GitHub
   - Settings → Pages
   - Source: Deploy from a branch
   - Branch: `main` → `/docs`
   - Save

4. **Accéder au flasher**
   - URL: `https://VOTRE_USERNAME.github.io/TouchAxe/`
   - Attendre 1-2 minutes pour le déploiement

### Option 2 : Branche gh-pages séparée

1. **Créer et basculer sur gh-pages**
   ```bash
   git checkout --orphan gh-pages
   git rm -rf .
   ```

2. **Copier le contenu du webflasher**
   ```bash
   cp -r webflasher/* .
   ```

3. **Commit et push**
   ```bash
   git add .
   git commit -m "Initial web flasher deployment"
   git push origin gh-pages
   ```

4. **Activer GitHub Pages**
   - Settings → Pages
   - Source: `gh-pages` → `/` (root)
   - Save

5. **Retourner sur main**
   ```bash
   git checkout main
   ```

## 📝 Commandes PowerShell pour Windows

```powershell
# Créer docs et copier les fichiers
New-Item -ItemType Directory -Path "docs" -Force
Copy-Item -Recurse -Force "webflasher\*" "docs\"

# Git
git add docs/
git commit -m "🚀 Add TouchAxe web flasher"
git push origin main
```

## 🔄 Mise à jour du firmware

Quand vous compilez une nouvelle version :

```powershell
# 1. Compiler
pio run

# 2. Copier les nouveaux binaires
Copy-Item ".pio\build\esp32-4827S043C\bootloader.bin" "webflasher\firmware\"
Copy-Item ".pio\build\esp32-4827S043C\partitions.bin" "webflasher\firmware\"
Copy-Item ".pio\build\esp32-4827S043C\firmware.bin" "webflasher\firmware\"

# 3. Mettre à jour la version dans index.html et manifest.json
# Éditer manuellement ou avec script

# 4. Copier vers docs si vous utilisez Option 1
Copy-Item -Recurse -Force "webflasher\*" "docs\"

# 5. Commit et push
git add webflasher/ docs/
git commit -m "📦 Update firmware to v1.0.X"
git push origin main
```

## 🎨 Personnalisation

### Modifier les couleurs
Éditez `webflasher/index.html` :
- Rouge électrique : `#ff0000`
- Fond : `#000` (noir)
- Texte : `#fff` (blanc)

### Changer les informations
Dans `index.html`, section `info-box` :
- Version
- Date de build
- Taille du firmware

### Modifier le manifest
Dans `manifest.json` :
- Nom du projet
- Version
- Offsets (NE PAS MODIFIER sans raison)

## 🔍 Vérification

Avant de déployer, testez localement :

```bash
# Option 1 : Python
cd webflasher
python -m http.server 8000

# Option 2 : Node.js (npx)
cd webflasher
npx serve

# Ouvrir http://localhost:8000 dans Chrome/Edge
```

## 📊 Tailles des fichiers

Les binaires dans `webflasher/firmware/` :
- `bootloader.bin` : ~15 KB
- `partitions.bin` : ~3 KB  
- `firmware.bin` : ~1.5 MB

**Total : ~1.52 MB** - Compatible avec GitHub Pages (limite 1 GB par repo)

## ⚠️ Notes importantes

1. **Navigateurs supportés** : Chrome, Edge, Opera uniquement (Web Serial API)
2. **HTTPS requis** : GitHub Pages fournit HTTPS automatiquement
3. **Offsets corrects** : Ne modifiez pas les offsets dans manifest.json
4. **Cache** : Utilisez Ctrl+F5 pour forcer le rechargement après mise à jour

## 🐛 Dépannage

**Page 404 après activation**
- Attendre 2-3 minutes
- Vérifier que les fichiers sont bien dans `docs/` ou à la racine de `gh-pages`
- Vérifier le nom du repo dans l'URL

**Firmware ne se charge pas**
- Vérifier les chemins dans manifest.json
- Ouvrir la console du navigateur (F12) pour voir les erreurs
- Vérifier que les 3 binaires sont présents dans `firmware/`

**Erreur "Failed to flash"**
- Vérifier la compatibilité du board (ESP32-S3)
- Essayer un autre port USB
- Installer les drivers CH340/CP2102

## 📱 Partage

Une fois déployé, partagez :
```
🚀 Flash TouchAxe Firmware:
https://VOTRE_USERNAME.github.io/TouchAxe/

⚡ ESP32-S3 Bitaxe Monitor
💰 Real-time Bitcoin Price
📊 Multi-device monitoring
```

---

Besoin d'aide ? Ouvrez une issue sur GitHub !
