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

$hasJobs = $false
foreach ($arg in $args) {
    if ($arg -match '^-j') {
        $hasJobs = $true
        break
    }
}

$dockerArgs = @('run', '--rm')
if (-not [Console]::IsInputRedirected) {
    $dockerArgs += '-it'
}
$dockerArgs += @('--volume', "${repoRoot}:/app", '--workdir', '/app', $image, 'bash', '-c')
if ($hasJobs) {
    $dockerArgs += 'exec make \"$@\"'
} else {
    $dockerArgs += 'exec make -j\"$(nproc)\" \"$@\"'
}
$dockerArgs += 'make'
$dockerArgs += $args

& docker @dockerArgs
exit $LASTEXITCODE
