# Olympic Ice Hockey Game

## **Description**
This project is a **3D simulation game** themed around **Ice Hockey**. The objective is to **collect scattred pucks within the ice rink** within the given time. The game environment includes **3D models** of the player, boundaries, ground, and other objects, with realistic rendering and animation.

## **Game Features**

### **Theme and Gameplay**
- **Theme**: Ice Hockey.
- **Objective**: Collect the pucks before time runs out.
- **Game End Conditions**:  
  - **Victory**: Player collects the pucks within the specified time.  
  - **Defeat**: Time runs out before collecting the pucks.  

### **3D Models**
- **Player**: Modeled with a head, body, and limbs using 8 primitives.  
- **Environment**:  
  - **Boundary Walls**: 3 walls, with at least 2 primitives each.  
  - **Ground**: Created with at least 1 primitive.  
  - **Pucks**: Modeled with at 3 primitives, disappearing upon collision.  
  - **Other Objects**:  
    - Crowd surrounding the rink.  
    - Nets, hockey sticks and fences surrounding the rink.

## **Technical Details**
- **Framework**: OpenGL and C++  

## **How to Run**
1. Compile the `.cpp` file in an OpenGL environment.  
2. Run the executable.  
3. To play the game using Keyboard Keys:
   - `Arrow keys` for moving the player right, left, forwards and backwards.
   - `'w', 's', 'a', 'd', 'q'` to change camera view for different persepectives.
   - `'p'` to to toggle different animations.
   - `'1', '2', '3'` to alternate between front view, side view and top view.
  
## **Acknowledgment**
This project is developed as an individual assignment for DMET 502: Computer Graphics during Winter 2024 at the German University in Cairo.
   

  
