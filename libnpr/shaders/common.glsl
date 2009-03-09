// HELPER FUNCTIONS FOR FRAGMENT SHADERS



struct LightingValues
{
    vec3 light_vec;
    vec3 normal;
    vec4 light_amb_col;
    vec4 light_diff_col;
    vec4 light_spec_col;
    vec4 mat_amb_col;
    vec4 mat_diff_col;
    vec4 mat_spec_col;
    float mat_shininess;
};

vec4 lambertianLight( LightingValues vals, float focus )
{
    vec3 view_dir = vec3(0.0, 0.0, 1.0);

    vec3 half_angle = normalize(view_dir + vals.light_vec);

    float diffuse = dot( vals.normal, vals.light_vec );
    float specular = dot( vals.normal, half_angle );
    
    vec4 lighting;
    lighting.x = 1.0;
    lighting.y = max(diffuse, 0.0);
    lighting.z = pow(max(specular, 0.0), vals.mat_shininess);
    
    vec4 out_col = lighting.x * vals.light_amb_col * vals.mat_amb_col + 
              lighting.y * vals.light_diff_col * vals.mat_diff_col +
              lighting.z * vals.light_spec_col * vals.mat_spec_col;
    
    return out_col;
}

vec4 lambertianLightDoubleSided( LightingValues vals, float focus )
{
    vec3 view_dir = vec3(0.0, 0.0, 1.0);
    
    vec3 half_angle = normalize(view_dir + vals.light_vec);
    
    float diffuse = abs(dot( vals.normal, vals.light_vec ));
    //diffuse = diffuse * (1.0 - focus) + 0.7 * focus;
    float specular = abs(dot( vals.normal, half_angle ));
    
    vec4 lighting;
    lighting.x = 1.0;
    lighting.y = max(diffuse, 0.0);
    lighting.z = pow(max(specular, 0.0), vals.mat_shininess);
    
    vec4 out_col = lighting.x * vals.light_amb_col * vals.mat_amb_col + 
    lighting.y * vals.light_diff_col * vals.mat_diff_col +
    lighting.z * vals.light_spec_col * vals.mat_spec_col;
    
    return out_col;
}

vec4 frontBackLight( LightingValues vals, float focus )
{
    vec3 view_dir = vec3(0.0, 0.0, 1.0);

    vec3 half_angle = normalize(view_dir + vals.light_vec);

	float back_amt = 0.3;
    float diffuse = dot( vals.normal, vals.light_vec );
    if (diffuse < 0.0)
    {
		diffuse *= -back_amt;
	}
    float specular = dot( vals.normal, half_angle );
    if (specular < 0.0)
    {
		specular *= -back_amt;
	}
    
    vec4 lighting;
    lighting.x = 0.5;
    lighting.y = max(diffuse, 0.0);
    lighting.z = pow(max(specular, 0.0), vals.mat_shininess);
    
    vec4 out_col = lighting.x * vals.light_amb_col * vals.mat_amb_col + 
              lighting.y * vals.light_diff_col * vals.mat_diff_col +
              lighting.z * vals.light_spec_col * vals.mat_spec_col;
    
    return out_col;
}

vec4 warmCoolLight( LightingValues vals, float focus )
{
    vec3 view_dir = vec3(0.0, 0.0, 1.0);

    vec3 half_angle = normalize(view_dir + vals.light_vec);
    vec4 light_col = vec4(1.0,1.0,0.7,1.0);

    float diffuse = dot( vals.normal, vals.light_vec );
    if (diffuse < 0.0)
    {
		diffuse *= -0.5;
		light_col = vec4(0.7,0.7,1.0,1.0);
	}
    float specular = dot( vals.normal, half_angle );
    if (specular < 0.0)
    {
		specular *= -0.5;
	}
    
    vec4 lighting;
    lighting.x = 0.5;
    lighting.y = max(diffuse, 0.0);
    lighting.z = pow(max(specular, 0.0), vals.mat_shininess);
    
    vec4 out_col = lighting.x * vals.light_amb_col * vals.mat_amb_col + 
              lighting.y * vals.light_diff_col * vals.mat_diff_col +
              lighting.z * vals.light_spec_col * vals.mat_spec_col;
    out_col *= light_col;
    
    return out_col;
}

vec4 colorWheel( float theta )
{
    vec4 outcol;
    float pi_3 = 3.1415926 / 3.0;
    float pi = 3.1415926;
    if (theta < 0.0)
    {
        theta = theta + pi;
    }

    if (theta < pi_3)
    {
        float red = (pi_3 - theta)/pi_3;
        float green = theta / pi_3;
        outcol = vec4(1.0,0.0,0.0,0.0)*red + vec4(0.0,1.0,0.0,0.0)*green;
    }
    else if (theta < 2.0*pi_3)
    {
        float green = (2.0*pi_3 - theta)/pi_3;
        float blue = (theta - pi_3) / pi_3;
        outcol = vec4(0.0,0.0,1.0,0.0)*blue + vec4(0.0,1.0,0.0,0.0)*green;
    }
    else
    {
        float blue = (3.0*pi_3 - theta)/pi_3;
        float red = (theta - 2.0*pi_3) / pi_3;
        outcol = vec4(0.0,0.0,1.0,0.0)*blue + vec4(1.0,0.0,0.0,0.0)*red;
    }
    outcol[3] = 1.0;
    return outcol;
}

