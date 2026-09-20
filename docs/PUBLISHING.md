# Initial GitHub upload

Suggested repository description:

> Work-in-progress C++ reconstruction of Silkroad Online's SR_GameServer, with native behavior research, regression tests, and evidence notes.

Create `vsro-server-decomp` under your GitHub account or organization. Select your intended visibility. Leave GitHub's README, .gitignore, and license initialization options off: the local repository already supplies the first two, and no license has been selected.

The local repository is initialized on `main`. From its root, review and commit the files:

```powershell
git status --short
git add .
git diff --cached --stat
git commit -m "Initial source reconstruction and documentation"
```

If Git requests an identity, set your own name and email with `git config user.name` and `git config user.email`, then retry the commit.

Replace `YOUR_OWNER` with your account or organization:

```powershell
git remote add origin https://github.com/YOUR_OWNER/vsro-server-decomp.git
git remote -v
git push -u origin main
```

Complete GitHub authentication if prompted. If `origin` already exists, inspect `git remote -v` and use `git remote set-url origin <correct-url>` if necessary. These instructions assume an empty remote repository.

For later updates:

```powershell
git add .
git commit -m "Describe the change"
git push
```

Build products and temporary files remain on disk but are excluded by `.gitignore`. `server.cfg` stays local; `server.example.cfg` is the versioned template. Avoid forcing ignored files into a commit.

Reference: [GitHub's instructions for adding locally hosted code](https://docs.github.com/en/migrations/importing-source-code/using-the-command-line-to-import-source-code/adding-locally-hosted-code-to-github).
