#version 450

layout(location=0) in vec3 inColor;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inPosition;
layout(location=3) in vec3 inBarycentric;

layout(location=0) out vec4 outColor;

uniform vec3 uLightPos; 
uniform vec3 uViewPos; 
uniform vec3 uLightColor;

void main() {
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * uLightColor;
  	
    // diffuse 
    vec3 norm = normalize(inNormal);
    vec3 uLightDir = normalize(uLightPos - inPosition);
    float diff = max(dot(norm, uLightDir), 0.1);
    vec3 diffuse = diff * uLightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(uViewPos - inPosition);
    vec3 reflectDir = reflect(-uLightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * uLightColor;  
        
    vec3 result = (ambient + diffuse + specular) * inColor;
    outColor = vec4(result, 1.0);

	if(inBarycentric.x < 0.01 || inBarycentric.y < 0.01 || inBarycentric.z < 0.01) {
	    outColor = outColor * 0.25;
	}
} 