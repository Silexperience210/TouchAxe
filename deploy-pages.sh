#!/bin/bash
# Script de déploiement GitHub Pages pour TouchAxe

echo "🚀 Déploiement de TouchAxe sur GitHub Pages..."

# Vérifier que nous sommes dans le bon répertoire
if [ ! -d "docs" ]; then
    echo "❌ Dossier docs non trouvé. Assurez-vous d'être à la racine du projet."
    exit 1
fi

# Copier les fichiers firmware dans docs/firmware/
echo "📁 Copie des fichiers firmware..."
mkdir -p docs/firmware
cp releases/v1.1/firmware.bin docs/firmware/
cp releases/v1.1/bootloader.bin docs/firmware/
cp releases/v1.1/partitions.bin docs/firmware/

# Vérifier que les fichiers sont présents
if [ ! -f "docs/firmware/firmware.bin" ]; then
    echo "❌ Erreur: firmware.bin non trouvé dans docs/firmware/"
    exit 1
fi

echo "✅ Fichiers firmware copiés avec succès"

# Commit et push des changements
echo "📤 Commit et push des changements..."
git add docs/
git commit -m "📄 Mise à jour GitHub Pages - Firmware V1.1 et interface de flash

- Ajout des fichiers binaires V1.1
- Interface web avec sélection de version
- Manifests esp-web-tools pour flash automatique
- Documentation mise à jour"

git push origin main

echo "🎉 Déploiement terminé !"
echo ""
echo "📋 Prochaines étapes :"
echo "1. Créer une release GitHub avec l'archive TouchAxe-V1.1-Firmware.zip"
echo "2. Vérifier que GitHub Pages fonctionne : https://silexperience210.github.io/TouchAxe/"
echo "3. Tester le flash automatique depuis l'interface web"