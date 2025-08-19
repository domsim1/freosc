# GitHub Action Workflow Testing Guide

## Workflow Overview
The `release.yml` workflow is designed to:
1. **Trigger**: On version tags matching `v*.*.*` (e.g., v1.0.0, v2.1.3)
2. **Build**: FreOSC VST for Windows and macOS in parallel
3. **Package**: Create ZIP archives with VST3, AU (macOS), and Standalone builds
4. **Release**: Create GitHub release with auto-generated notes and upload assets

## Manual Testing Steps

### 1. Test Tag Creation (Dry Run)
```bash
# Create a test tag locally (don't push yet)
git tag v1.0.1
git tag -l  # List tags to verify

# Check what the workflow would trigger on
echo "Tag: v1.0.1 would trigger workflow"
echo "Version extracted: 1.0.1"

# Remove test tag
git tag -d v1.0.1
```

### 2. Validate Build Paths
The workflow expects these build outputs:

**Windows:**
- VST3: `build/FreOSC-VST_artefacts/Release/VST3/FreOSC.vst3`
- Standalone: `build/FreOSC-VST_artefacts/Release/Standalone/FreOSC.exe`

**macOS:**
- VST3: `build/FreOSC-VST_artefacts/Release/VST3/FreOSC.vst3`
- AU: `build/FreOSC-VST_artefacts/Release/AU/FreOSC.component`
- Standalone: `build/FreOSC-VST_artefacts/Release/Standalone/FreOSC.app`

### 3. JUCE Version Used
- **Version**: 7.0.12 (latest stable at time of creation)
- **Auto-cached**: JUCE is downloaded and cached per OS
- **Fallback**: If 7.0.12 becomes unavailable, update the version in the workflow

### 4. Test Workflow Components

#### Matrix Strategy
```yaml
strategy:
  matrix:
    os: [windows-latest, macos-latest]
```
This runs builds in parallel on both platforms.

#### Version Extraction
```bash
# From tag refs/tags/v1.2.3
VERSION=${GITHUB_REF#refs/tags/v}  # Results in "1.2.3"
TAG=${GITHUB_REF#refs/tags/}       # Results in "v1.2.3"
```

#### Artifact Naming
- Windows: `FreOSC-Windows-v1.2.3.zip`
- macOS: `FreOSC-macOS-v1.2.3.zip`

### 5. Expected Release Output

When you push tag `v1.0.1`, the workflow will create:

**GitHub Release:**
- Title: "FreOSC v1.0.1"
- Tag: "v1.0.1" 
- Auto-generated release notes from commits since last tag
- Installation instructions for both platforms

**Release Assets:**
- `FreOSC-Windows-v1.0.1.zip` (~5-15 MB)
- `FreOSC-macOS-v1.0.1.zip` (~10-25 MB)

## First Test Deployment

### Step 1: Commit the workflow
```bash
git add .github/
git commit -m "Add GitHub Action for automated VST builds and releases"
git push origin master
```

### Step 2: Create and push a test tag
```bash
git tag v1.0.1
git push origin v1.0.1
```

### Step 3: Monitor the workflow
1. Go to your GitHub repository
2. Click "Actions" tab
3. Watch the "Build and Release FreOSC VST" workflow run
4. Check both build jobs complete successfully
5. Verify release is created with assets

### Step 4: Verify the release
1. Go to "Releases" section of your repository
2. Confirm "FreOSC v1.0.1" release exists
3. Download and test both ZIP files
4. Verify VST3 files work in a DAW

## Troubleshooting Common Issues

### Build Failures
- **JUCE not found**: Check JUCE download links in workflow
- **CMake errors**: Verify CMakeLists.txt compatibility
- **Missing dependencies**: Check runner OS compatibility

### Release Creation Failures
- **Permission denied**: Ensure GITHUB_TOKEN has proper permissions
- **Duplicate release**: Delete existing release/tag if re-running

### Asset Upload Failures
- **File not found**: Check build artifact paths match expected locations
- **Size limits**: GitHub has 2GB per file, 25GB per repository limit

## Workflow Permissions Required

The workflow uses the default `GITHUB_TOKEN` which needs:
- `contents: write` - To create releases
- `actions: read` - To download artifacts
- `packages: read` - For caching

These are typically available by default in GitHub repositories.

## Future Enhancements

### Code Signing (Optional)
- **Windows**: Add Authenticode signing for .exe/.vst3
- **macOS**: Add Apple Developer ID signing and notarization

### Additional Formats
- **AAX**: Requires Pro Tools SDK and Avid signing
- **LV2**: Linux plugin format support

### Build Optimization
- **Caching**: More aggressive caching of build dependencies
- **Parallel Jobs**: Increase parallel build jobs if needed
- **Build Matrix**: Add different architectures (ARM64, etc.)

## Manual Release Process (Fallback)

If the automated workflow fails, you can still create releases manually:

1. Build locally on each platform
2. Create ZIP archives with same naming convention
3. Create GitHub release manually
4. Upload ZIP files as release assets

This workflow provides a robust, automated solution for FreOSC VST releases while maintaining flexibility for manual intervention when needed.