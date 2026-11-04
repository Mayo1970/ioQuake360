# Runs make with the given arguments in the toolchain container, with the repo root at /app.
$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$image = $env:Q360_IMAGE
if ([string]::IsNullOrEmpty($image)) {
    $image = 'ioquake360/xsdk-toolchain'
}

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    [Console]::Error.WriteLine('Error: docker was not found. Once Docker is available, run: docker build -t ioquake360/xsdk-toolchain docker/xsdk-toolchain')
    exit 1
}

& docker image inspect $image *> $null
if ($LASTEXITCODE -ne 0) {
    [Console]::Error.WriteLine("Error: Docker image $image is missing or unavailable. Run: docker build -t ioquake360/xsdk-toolchain docker/xsdk-toolchain")
    exit 1
}

$jobs = '-j\"$(nproc)\"'
foreach ($arg in $args) {
    if ($arg -match '^(-j|--jobs)') {
        $jobs = ''
        break
    }
}

# PowerShell 5.1 passes embedded quotes to native programs unescaped, hence the \" in the bash command.
$dockerArgs = @('run', '--rm', '--volume', "${repoRoot}:/app", '--workdir', '/app', $image,
    'bash', '-c', "exec make $jobs \`"`$@\`"", 'make') + $args

& docker @dockerArgs
exit $LASTEXITCODE
