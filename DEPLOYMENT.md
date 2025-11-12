# 🚀 Guide de Déploiement GitHub

Ce guide vous explique comment déployer TouchAxe sur GitHub avec GitHub Pages pour le Web Flasher.

## 📋 Étapes de Déploiement

### 1️⃣ Créer le Repository GitHub

1. **Aller sur GitHub**: https://github.com/new
2. **Nom du repository**: `TouchAxe`
3. **Description**: `Professional Bitcoin Mining Dashboard for ESP32-S3`
4. **Visibilité**: Public (requis pour GitHub Pages gratuit)
5. **NE PAS** initialiser avec README, .gitignore ou LICENSE (déjà créés)
6. **Cliquer** sur "Create repository"

### 2️⃣ Lier le Repository Local

Dans le terminal PowerShell du projet:

```powershell
# Ajouter l'origine GitHub (remplacer USERNAME par votre nom d'utilisateur)
git remote add origin https://github.com/Silexperience210/TouchAxe.git

# Vérifier que c'est bien ajouté
git remote -v

# Pousser le code vers GitHub
git push -u origin main
```

### 3️⃣ Activer GitHub Pages

1. **Aller dans Settings** du repository
2. **Cliquer sur "Pages"** dans le menu latéral
3. **Source**: Sélectionner `Deploy from a branch`
4. **Branch**: Sélectionner `main` et folder `/root`
5. **Cliquer** sur "Save"
6. **Attendre** 1-2 minutes pour le déploiement

### 4️⃣ Vérifier le Déploiement

Votre site sera accessible à:
```
https://silexperience210.github.io/TouchAxe/
```

**Test du Web Flasher**:
1. Ouvrir le lien ci-dessus dans Chrome/Edge
2. Connecter un ESP32-S3 via USB
3. Cliquer sur "Install TouchAxe Firmware"
4. Sélectionner le port COM
5. Le flash devrait démarrer automatiquement

## 🔧 Configuration Optionnelle

### Actions GitHub (Auto-Build)

Les GitHub Actions sont déjà configurées dans `.github/workflows/build.yml`

**Ce qu'elles font**:
- ✅ Build automatique à chaque push sur `main`
- ✅ Mise à jour automatique des binaires dans `firmware/`
- ✅ Création de releases automatiques pour les tags `v*`

**Pour créer une release**:
```powershell
git tag v1.0.0
git push origin v1.0.0
```

### Permissions pour GitHub Actions

Pour que les Actions puissent commiter les binaires:

1. **Settings** → **Actions** → **General**
2. **Workflow permissions**
3. Cocher ✅ **Read and write permissions**
4. **Save**

## 📝 Commandes Git Utiles

### Push de Nouvelles Modifications

```powershell
# Vérifier les changements
git status

# Ajouter les fichiers modifiés
git add .

# Créer un commit
git commit -m "Description des changements"

# Pousser vers GitHub
git push
```

### Créer une Nouvelle Version

```powershell
# Build du firmware
pio run

# Copier les nouveaux binaires
Copy-Item ".pio\build\esp32-4827S043C\*.bin" -Destination "firmware\"

# Commit et tag
git add firmware/
git commit -m "Update firmware to v1.1.0"
git tag v1.1.0
git push origin main --tags
```

### Annuler des Changements

```powershell
# Annuler les modifications non commitées
git restore <fichier>

# Annuler le dernier commit (garde les changements)
git reset --soft HEAD~1

# Annuler le dernier commit (supprime les changements)
git reset --hard HEAD~1
```

## 🎨 Personnalisation du README

Le README contient des badges avec votre username. Vérifiez:

- [ ] Badges GitHub (stars, forks, license)
- [ ] Liens vers le repository
- [ ] URL du Web Flasher
- [ ] Informations de contact

## 🔐 Sécurité

**Ne JAMAIS commiter**:
- ❌ Mots de passe WiFi
- ❌ Clés API privées
- ❌ Identifiants personnels

Ces fichiers sont déjà exclus dans `.gitignore`:
- `secrets.h`
- `credentials.h`
- `config_private.h`

## ✅ Checklist Finale

Avant de rendre le projet public:

- [ ] Commit initial créé et poussé
- [ ] GitHub Pages activé
- [ ] Web Flasher testé et fonctionnel
- [ ] README.md complet et à jour
- [ ] LICENSE présent (MIT)
- [ ] .gitignore configuré
- [ ] GitHub Actions activées
- [ ] Permissions Actions configurées
- [ ] URL du site dans le README
- [ ] Description du repository GitHub

## 🆘 Dépannage

### Le Web Flasher ne fonctionne pas

**Problème**: Erreur CORS ou fichiers non trouvés

**Solution**:
1. Vérifier que GitHub Pages est activé
2. Attendre 5 minutes après activation
3. Vider le cache du navigateur (Ctrl+F5)
4. Vérifier que les fichiers sont dans `firmware/`

### Les Actions échouent

**Problème**: Build error ou permissions

**Solution**:
1. Vérifier les logs dans Actions tab
2. Activer "Read and write permissions"
3. Vérifier que `platformio.ini` est valide

### Le repository est trop gros

**Problème**: Git refuse le push (>100MB)

**Solution**:
```powershell
# Retirer les gros fichiers du staging
git rm --cached .pio -r
git rm --cached .vscode -r

# Commit et push
git commit -m "Remove build artifacts"
git push
```

## 📧 Support

Questions ou problèmes ?
- **GitHub Issues**: https://github.com/Silexperience210/TouchAxe/issues
- **GitHub Discussions**: https://github.com/Silexperience210/TouchAxe/discussions

---

**Prochaine étape**: [Contribuer au projet](CONTRIBUTING.md)
