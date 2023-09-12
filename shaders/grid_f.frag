#version 330 core

out vec4 FragColor;

uniform mat4 transform;
uniform float resolution;

void main()
{
    // Define the size of the grid
    int gridSize = 9;
    
    // Calculate the step size for the grid lines
    float stepSize = 2.0 / float(gridSize);

    // Calculate the UV coordinates of the current fragment
    vec2 uv = (gl_FragCoord * transform).xy / vec2(resolution, resolution); // Adjust the resolution as needed
    float scale_x = transform[0][0];
    float scale_y = transform[1][1];

    // Calculate the coordinates of the current fragment in the [-1, 1] range
    float fragCoordX = (uv.x - 0.5) * (2.0 / scale_x) - 1.0;
    float fragCoordY = (uv.y - 0.5) * (2.0 / scale_y) - 1.0;
    
    // Colors for grid lines
    vec3 gridColor = vec3(0.4, 0.4, 0.4);
    vec3 gridColorThick = vec3(0.2, 0.2, 0.2);

    bool isInsideGridHorizontally = (fragCoordX >= -2.0 && fragCoordX <= 0.0);
    bool isInsideGridVertically = (fragCoordY >= -2.0 && fragCoordY <= 0.0);
    bool isInsideGrid = isInsideGridHorizontally && isInsideGridVertically;

    // Determine if the current fragment is on a thicker grid line
    bool isThickLineX = isInsideGrid && mod(fragCoordX, (stepSize + 0.0007) * 3) <= 0.01;
    bool isThickLineY = isInsideGrid && mod(fragCoordY, (stepSize + 0.0007) * 3) <= 0.01;

    // Check if the current fragment is on a grid line
    // 0.0007 to shift the grid lines a bit to the left
    bool isGridX = isInsideGrid && mod(fragCoordX, stepSize + 0.0007) <= 0.005;
    bool isGridY = isInsideGrid && mod(fragCoordY, stepSize + 0.0007) <= 0.005;
    
    // Set the fragment color based on whether it's a grid line or a thicker grid line
    if ((isThickLineX) || (isThickLineY)) {
        FragColor = vec4(gridColorThick, 1.0);
    } else if (isGridX || isGridY) {
        FragColor = vec4(gridColor, 1.0);
    } else {
        FragColor = vec4(240.f/255.f, 235.f/255.f, 227.f/255.f, 1.0);
    }
}

