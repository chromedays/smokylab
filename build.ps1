.\FBuild.exe Exe-smokylab
.\FBuild.exe Exe-smokylab -compdb

function buildComputeShader {
    param (
        $name
    )
    $fxc = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\fxc.exe"
    & ${fxc} "shaders/$name.hlsl" -E vert -T vs_5_0 -nologo -Fo "bin/$name.vs.cso"
    & ${fxc} "shaders/$name.hlsl" -E frag -T ps_5_0 -nologo -Fo "bin/$name.fs.cso"
}

buildComputeShader "brdf"
buildComputeShader "sky"
buildComputeShader "exposure"
buildComputeShader "wireframe"
buildComputeShader "fst"