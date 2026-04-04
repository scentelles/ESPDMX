# DMXESP Web Deployment - Statut

✅ **FAIT**: Frontend React compilé avec succès  
📍 **À FAIRE**: Uploader les fichiers vers l'ESP32

## Fichiers Générés ✅

Chemin: `C:\Temp\DMXESP\firmware\data\web\`

```
index.html (0.75 kB)
assets/
  ├── index-CfLXHOyW.css (17.14 kB)
  ├── index-CSlpOA-3.js (70.98 kB)
  └── react-F9Y4d3HK.js (140.88 kB)
```

**Total: 4 fichiers (0.22 MB)** - Prêt pour l'ESP32

---

## Étape 2: Upload vers l'ESP32

Choisissez une des options ci-dessous:

### ✨ **Option A: VS Code PlatformIO (PLUS FACILE)**

1. Ouvrez VS Code
2. Appuyez sur `Ctrl + Shift + P`
3. Tapez: `PlatformIO: Upload File System Image`
4. VS Code gérera tout automatiquement

**Avantage**: Aucune installation supplémentaire

---

### **Option B: Terminal PowerShell (Notre script)**

```powershell
# D'abord, installez PlatformIO CLI:
pip install platformio

# Puis lancez l'upload:
cd C:\Temp\DMXESP\firmware
pio run --target uploadfs
```

Ou utilisez notre script:
```powershell
cd C:\Temp\DMXESP
.\Upload-Web.ps1
```

---

### **Option C: Terminal classique (CMD)**

```bash
cd C:\Temp\DMXESP\firmware
platformio run --target uploadfs
```

---

## Après l'Upload

### Sur l'ESP32:
1. **Redémarrez** l'ESP32 (débranchez/rebranchez)
2. **Ouvrez un navigateur** et allez à:
   - `http://192.168.4.1` (mode AP par défaut)
   - Ou votre adresse IP configurée dans `config.h`

3. **Vous devriez voir** l'interface web DMXESP avec:
   - Page utilisateur (contrôle de scènes, lumières, effets)
   - Panel admin (configuration des fixtures)

---

## Troubleshooting

### Erreur: `{"error":"Not found"}`
→ Les fichiers web ne sont pas uploadés sur le SPIFFS  
→ **Solution**: Refaire l'upload avec une des 3 options ci-dessus

### Erreur: `Connection refused` ou pas de réponse
→ L'ESP32 n'est pas accessible  
→ **Vérifiez**:
- WiFi est connecté (vérifiez config.h)
- Adresse IP correcte
- ESP32 est allumé (LED clignotante)

### Page web blanche mais pas d'erreur
→ Fichiers uploadés mais JavaScript ne s'exécute pas  
→ **Solution**: Presser `F12` pour voir la console de débogage

### `Cannot read module 'platformio'`
→ PlatformIO n'est pas installé  
→ **Solution**: 
```bash
pip install platformio
```

---

## Prochaines Fois

Après la première installation, pour déployer à nouveau:

```powershell
# Reconstruire l'interface + uploader
cd C:\Temp\DMXESP
./deploy-web.ps1

# Ou juste uploader (sans reconstruction)
./Upload-Web.ps1
```

---

## État Actuel Récapitulatif

| Étape | Statut | Détails |
|-------|--------|---------|
| Build React | ✅ Fait | Fichiers générés en: `firmware/data/web/` |
| Copy files | ✅ Fait | 4 fichiers (0.22 MB) copiés |
| **Install PlatformIO** | ⚠️ À faire | `pip install platformio` |
| **Upload to ESP32** | ⏳ En attente | Choisir l'option A, B ou C ci-dessus |
| Test web interface | ⏳ Après upload | Accès à `http://192.168.4.1` |

---

## Commande Recommandée

```powershell
# La plus simple : utiliser VS Code directement
# Ctrl+Shift+P → "PlatformIO: Upload File System Image"
```
