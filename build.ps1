.\FBuild.exe Exe-smokylab
.\FBuild.exe Exe-smokylab -compdb

function buildComputeShader {
    param (
        $name
    )
    $fxc = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe"
    
    $vsoutput = "bin/$name.vs.cso"
    $fsoutput = "bin/$name.fs.cso"

    $source = "shaders/$name.hlsl"

    $shouldCompile = $(-Not $(Test-Path -Path $vsoutput -PathType Leaf))
    if (-Not $shouldCompile) {
        $secondsdiff =
            ((Get-Item $source).LastWriteTime -
             (Get-Item $vsoutput).LastWriteTime).TotalSeconds
        if ($secondsdiff -gt 0) {
            $shouldCompile = $true            
        }
    }

    if ($shouldCompile) {
        & ${fxc} $source -E vert -T vs_5_0 -nologo -Fo $vsoutput
        & ${fxc} $source -E frag -T ps_5_0 -nologo -Fo $fsoutput
    }
}

buildComputeShader "brdf"
buildComputeShader "new_brdf"
buildComputeShader "sky"
buildComputeShader "exposure"
buildComputeShader "wireframe"
buildComputeShader "fst"
buildComputeShader "oit_accum"
buildComputeShader "oit_composite"
buildComputeShader "ssao"