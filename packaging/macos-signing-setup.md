# macOS Code Signing & Notarization Setup

This guide covers how to configure code signing for the JQ Gunk macOS CI/CD build.

## Prerequisites

- An [Apple Developer Program](https://developer.apple.com/programs/) membership ($99/year)
- A Mac with Keychain Access (for certificate export)
- Admin access to the GitHub repository settings

## 1. Create a Developer ID Certificate

1. Go to [developer.apple.com/account](https://developer.apple.com/account)
2. Navigate to **Certificates, Identifiers & Profiles** > **Certificates**
3. Click **+** to create a new certificate
4. Select **Developer ID Application** and click Continue
5. Follow the prompts to upload a Certificate Signing Request (CSR):
   - Open **Keychain Access** on your Mac
   - Go to **Keychain Access** > **Certificate Assistant** > **Request a Certificate From a Certificate Authority**
   - Enter your email, select **Saved to disk**, and click Continue
   - Upload the resulting `.certSigningRequest` file
6. Download the generated certificate (`.cer`) and double-click it to install in Keychain Access

## 2. Export the Certificate as .p12

1. Open **Keychain Access**
2. In the **login** keychain, find your **Developer ID Application** certificate under **My Certificates**
3. Right-click the certificate and select **Export**
4. Choose **Personal Information Exchange (.p12)** as the format
5. Set a strong password when prompted — you will need this for the `MACOS_CERTIFICATE_PASSWORD` secret

## 3. Base64-Encode the Certificate

Run this in Terminal to copy the encoded certificate to your clipboard:

```bash
base64 -i certificate.p12 | pbcopy
```

This is the value for the `MACOS_CERTIFICATE` secret.

## 4. Find Your Signing Identity

Run this in Terminal:

```bash
security find-identity -v -p codesigning
```

Look for the line containing **Developer ID Application**. The full string including your name and team ID is your signing identity, e.g.:

```
Developer ID Application: Your Name (XXXXXXXXXX)
```

This is the value for the `MACOS_SIGNING_IDENTITY` secret.

## 5. Find Your Team ID

Your 10-character Team ID appears in parentheses in your signing identity above. You can also find it at [developer.apple.com/account](https://developer.apple.com/account) under **Membership Details**.

This is the value for the `APPLE_TEAM_ID` secret.

## 6. Create an App-Specific Password

1. Go to [appleid.apple.com](https://appleid.apple.com)
2. Sign in and navigate to **Sign-In and Security** > **App-Specific Passwords**
3. Click **Generate an app-specific password**
4. Name it something like `github-notarization`
5. Copy the generated password

This is the value for the `APPLE_APP_PASSWORD` secret. Your Apple ID email is the value for `APPLE_ID`.

## 7. Add Secrets to GitHub

Go to your repository on GitHub:

1. Navigate to **Settings** > **Secrets and variables** > **Actions**
2. Click **New repository secret** and add each of the following:

| Secret Name | Value |
|---|---|
| `MACOS_CERTIFICATE` | Base64-encoded `.p12` contents (from step 3) |
| `MACOS_CERTIFICATE_PASSWORD` | Password you set when exporting the `.p12` (from step 2) |
| `MACOS_SIGNING_IDENTITY` | Full identity string, e.g. `Developer ID Application: Your Name (XXXXXXXXXX)` (from step 4) |
| `APPLE_ID` | Your Apple ID email address |
| `APPLE_APP_PASSWORD` | App-specific password (from step 6) |
| `APPLE_TEAM_ID` | Your 10-character team ID (from step 5) |

## 8. Verify

Push a tag or trigger the workflow manually:

```bash
git tag v0.0.0-test
git push origin v0.0.0-test
```

Check the Actions tab on GitHub. The signing and notarization steps should complete successfully. You can verify the result by downloading the DMG and running:

```bash
spctl --assess --type open --context context:primary-signature -v JQGunk-macos.dmg
codesign --verify --deep --strict --verbose=2 "JQ Gunk.vst3"
```

## Troubleshooting

- **"The specified item could not be found in the keychain"** — The certificate was not imported correctly. Ensure `MACOS_CERTIFICATE` is the full base64 output with no line breaks or truncation.
- **"Unable to upload your app for notarization"** — Check that `APPLE_APP_PASSWORD` is an app-specific password, not your regular Apple ID password.
- **Notarization rejected** — Run `xcrun notarytool log <submission-id> --apple-id ... --password ... --team-id ...` to see the detailed rejection reason. Common causes: missing hardened runtime (`--options runtime`) or unsigned nested binaries.
- **"Developer ID Application" certificate not available** — You must be enrolled in the Apple Developer Program. Free accounts only get development certificates, which are not valid for distribution.
