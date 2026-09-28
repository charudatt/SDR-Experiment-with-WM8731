# GitHub Web Upload Guide

This guide is for uploading the project to GitHub using only the web browser. No Git command line is required.

## 1. Recommended repository name

Use:

```text
SDR-Experiment-with-WM8731
```

Suggested description:

```text
Experimental ESP32-S3 + WM8731 audio codec platform for Amateur Radio SDR development and hardware validation.
```

## 2. Create the repository

1. Sign in to GitHub.
2. Open the **New repository** page.
3. Enter `SDR-Experiment-with-WM8731` as the repository name.
4. Enter the description above.
5. Choose **Public** if you want the project to be publicly visible.
6. Select **Add a README file**.
7. Create the repository.

The repository should now have a `main` branch and a README.

## 3. Upload this package

After creating the repository:

1. Open the repository main page.
2. Click **Add file**.
3. Choose **Upload files**.
4. Extract the supplied project ZIP on your Windows computer.
5. Open the extracted `SDR-Experiment-with-WM8731` folder.
6. Select **all files and folders inside that folder**.
7. Drag them into the GitHub upload area.
8. Check that GitHub shows the folders `docs`, `hardware`, `schematic` and `tools`.
9. Scroll to the commit section.
10. Use this commit message:

```text
Add initial WM8731 validation platform and REV-S7
```

11. If this is your personal repository and you want the first upload directly on the main branch, keep the option to commit directly to `main`.
12. Click **Commit changes**.

GitHub's browser upload supports uploading multiple files at once; browser uploads are limited to 25 MiB per individual file. citeturn0search9

## 4. Check the result

The repository root should look approximately like this:

```text
SDR-Experiment-with-WM8731
│
├── README.md
├── GITHUB_WEB_UPLOAD_GUIDE.md
│
├── docs
│   └── README.md
│
├── hardware
│   └── README.md
│
├── schematic
│   └── README.md
│
└── tools
    ├── README.md
    └── REV-S7
        ├── README.md
        ├── config.h
        ├── SDR_WM8731_ESP32S3_STEP2A2_REV_S7.ino
        └── doc
            └── REV-S7.md
```

## 5. Why the folders contain README files

Git tracks files rather than empty directories. The README files therefore establish the intended folder structure and explain what belongs in each area.

## 6. Adding the schematic later

When you have the final schematic PDF:

1. Open the `schematic` folder.
2. Click **Add file → Upload files**.
3. Upload the PDF.
4. Use a commit message such as:

```text
Add initial hardware schematic
```

## 7. Adding photographs later

Create an appropriate folder under `hardware`, for example:

```text
hardware/board-overview/
```

Then upload the photographs there. Keep filenames descriptive, for example:

```text
board-top.jpg
board-bottom.jpg
wm8731-module.jpg
esp32-s3-module.jpg
si5351-module.jpg
```

## 8. Adding a new validation tool

Do not replace REV-S7.

For the next experiment create:

```text
tools/REV-S8/
├── README.md
├── <source files>
└── doc/
    └── REV-S8.md
```

This keeps every successful hardware validation reproducible.

## 9. Recommended commit style

Keep commit messages short and descriptive:

```text
Add REV-S8 ADC validation
Add WM8731 schematic
Add board photographs
Update project documentation
Fix REV-S8 touch calibration
```

## 10. Important safety / reproducibility rule

Never upload passwords, API keys, private credentials or other secrets. GitHub specifically recommends keeping sensitive information out of repositories. citeturn0search9turn0search10

## 11. Official GitHub help

GitHub's current documentation for creating repositories is available from the official GitHub Docs. The browser-based file-upload procedure is also documented there. 
