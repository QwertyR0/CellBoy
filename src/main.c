#include <gb/gb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gbdk/console.h>
#include <gbdk/font.h>
#include "assets/titleScreen_data.c"
#include "assets/titleScreen_map.c"
#include "assets/select.c"
#include "assets/settings_map.c"
#include "assets/settings_data.c"
#include "assets/Cells.c"
extern uint8_t savedGame[50][50];
extern uint8_t savedWidth;
extern uint8_t savedHeight;
extern uint8_t fingerPrint[3];

uint8_t promptNumber(char prompt[]);
void processInputs();
void drawGame();
void updateGame(uint8_t state);
void saveGame();
void resetSavedGame();
uint8_t checkIfSavedMap();
void loadSavedGame();
void updateCell(int x, int y, uint8_t cellI, uint8_t id, uint8_t dir, uint8_t state);
uint8_t pushCell(int x, int y, uint8_t rot, uint8_t isMover);
uint8_t isPerpendicurlar(uint8_t rot1, uint8_t rot2);
void adjustInputDelay();

// fill_bkg_rect(0, 0, 20, 18, 0x00); is good for clearing bkg

struct gamec {
    uint8_t width;
    uint8_t height;
    uint8_t curx;
    uint8_t cury;
    uint8_t camx;
    uint8_t camy;
    uint8_t selectedCell;
    uint8_t running;
    uint8_t tickRate;
    uint8_t settings;
    uint8_t updateState; // 0 auto, 1 step
    uint8_t menuCursor;
    uint8_t lastOneLength;
    uint8_t lastRot;
    uint8_t shouldPrintRot;
    uint8_t firsTimeManu;
} game;
                                //Mo Ge CW ACW OW MW TR  WA  EN
const uint8_t cellRSprites[10] = {0, 4, 8,  12, 17, 19, 20, 21, 22};
const uint8_t cellDSprites[10] = {1, 5, 9,  13, 18, 19, 20, 21, 23};
const uint8_t cellLSprites[10] = {2, 6, 10, 14, 17, 19, 20, 21, 24};
const uint8_t cellUSprites[10] = {3, 7, 11, 15, 18, 19, 20, 21, 22};

uint8_t toX, toY;
uint8_t input_delay = 2;
uint8_t map[50][50];
uint8_t resMap[50][50];

void main() {
    ENABLE_RAM_MBC1;
    // load splash screen
    set_bkg_data(0, 104, titleScreen_data);
    set_bkg_tiles(0, 0, 20, 18, titleScreen_map);

    // load cursor:
    set_sprite_data(0, 1, selectSprite);

    SHOW_BKG;
    DISPLAY_ON;
    SHOW_SPRITES; // add this line to show the sprite

    waitpad(J_START);
    if(checkIfSavedMap() == 1){
        loadSavedGame();
        printf(" ");
        cls();
        delay(10);
    } else {
        game.width = promptNumber("Width:");
        uint8_t input = joypad();
        while (input == J_START) {
            input = joypad();
        }
        cls();
        game.height = promptNumber("Height:");
        delay(10);
        cls();
    }

    game.running = 0;
    game.tickRate = 2;
    game.menuCursor = 1;
    game.lastOneLength = 5;
    game.lastRot = 0;
    game.shouldPrintRot = 1;
    game.firsTimeManu = 0;

    if(game.width >= 20){
        toX = 20;
    } else {
        toX = game.width;
    }

    if(game.height >= 18){
        toY = 18;
    } else {
        toY = game.height;
    }

    adjustInputDelay();

    // load Cells:
    set_bkg_data(102, 25, Cells);

    while(1){
        processInputs();
        if(game.running == 1){
            static uint8_t timer = 0;
            if(timer == 0){
                updateGame(0);
                timer = game.tickRate*5;
            }
            timer--;
        }
        drawGame();
    }

    DISABLE_RAM_MBC1;
}

uint8_t promptNumber(char prompt[]) {
    uint8_t number = 1;
    uint8_t i = 0;
    uint8_t input = 0;
    uint8_t last_input = 0;

    // draw prompt
    gotoxy(10 - strlen(prompt)/2, 8);
    printf(prompt);

    while (input != J_START) {
        input = joypad();
        if (input == J_LEFT && last_input != J_LEFT && number > 1) {
            number--;
        } else if (input == J_RIGHT && last_input != J_RIGHT && number < 50) {
            number++;
        }
        last_input = input;

        char number_str[3];
        sprintf(number_str, "%d", number);
        gotoxy(10 - strlen(number_str)/2, 9);
        printf("%d", number);
        memset(number_str, 0, 3);
    }

    return number;
}

void drawGame(){
    if(game.settings == 1){
        set_sprite_tile(0, 0);
        uint8_t cursorPos = game.menuCursor == 1 ? 47 : game.menuCursor == 2 ? 78 : 90;
        move_sprite(0, 42 + 8, cursorPos + 16);
        wait_vbl_done();
        char tickRateStr[7] = "";
        sprintf(tickRateStr, game.updateState == 0 ? "automa" : "manual");
        uint8_t strLength = strlen(tickRateStr);
        gotoxy(10 - strLength/2, 7);
        printf(tickRateStr);
    } else {
        unsigned int x, y;
        if(game.running == 0){
            set_sprite_tile(0, 0);
            move_sprite(0, game.curx*8 + 8, game.cury*8 + 16);
        } else {
            move_sprite(0, 0, 0);
        }

        for (y = 0; y < toY; y++) {
            for (x = 0; x < toX; x++) {
                // if doesn't collide with the cell text:
                if(game.running != 0 || ((y != 0 || x < 20 - game.lastOneLength) && (y != 1 || x != 19))){
                    uint8_t cell = (map[x + game.camx][y + game.camy] & 0x3F);
                    if (cell == 0 && get_bkg_xy_addr(x, y) != 0){
                        set_bkg_tile_xy(x, y, 0);
                    } else if (cell != 0){
                        uint8_t cellToRender = 102;
                        uint8_t dir = (map[x + game.camx][y + game.camy] & 0xC0) >> 6;
                        if(dir == 0){
                            cellToRender += cellRSprites[cell - 1];
                        } else if(dir == 1){
                            cellToRender += cellDSprites[cell - 1];
                        } else if(dir == 2){
                            cellToRender += cellLSprites[cell - 1];
                        } else if(dir == 3){
                            cellToRender += cellUSprites[cell - 1];
                        }

                        // Check if it's already there:
                        if(get_bkg_xy_addr(x, y) != cellToRender){
                            set_bkg_tile_xy(x, y, cellToRender);
                        }
                    }
                }
            } 
        }

        // NOTE: can remove this anytime
        wait_vbl_done();
        if(game.running == 0 && game.settings == 0){
            char cell_str[25] = "";
            // strcat with using the last 6 bits:
            switch(game.selectedCell & 0x3F) {
                case 0:
                    strcat(cell_str, "Empty");
                    break;
                case 1:
                    strcat(cell_str, "Mover");
                    break;
                case 2:
                    strcat(cell_str, "Generator");
                    break;
                case 3:
                    strcat(cell_str, "RotatorCW");
                    break;
                case 4:
                    strcat(cell_str, "RotatorACW");
                    break;
                case 5:
                    strcat(cell_str, "One Way");
                    break;
                case 6:
                    strcat(cell_str, "Multi Way");
                    break;
                case 7:
                    strcat(cell_str, "Trash");
                    break;
                case 8:
                    strcat(cell_str, "Wall");
                    break;
                case 9:
                    strcat(cell_str, "enemy");
                    break;
                default:
                    strcat(cell_str, "Unknown");
                    break;
            }

            uint8_t textL = strlen(cell_str);
            uint8_t dir = (game.selectedCell & 0xC0) >> 6;
            // check if the text is same as the last one:
            if(game.lastOneLength != textL){
                gotoxy(20 - game.lastOneLength, 0);
                for(uint8_t i = 0; i < game.lastOneLength; i++){
                    printf(" ");
                }
                game.lastOneLength = textL;
            }
            gotoxy(20 - textL, 0);
            printf(cell_str);
            if(game.lastRot != dir || game.shouldPrintRot == 1){
                gotoxy(19, 1);
                printf(dir == 0 ? "R" : dir == 1 ? "D" : dir == 2 ? "L" : "U");
                game.lastRot = dir;
                if(game.shouldPrintRot == 1){
                    game.shouldPrintRot = 0;
                }
            }
        } else if(get_bkg_tile_xy(20 - game.lastOneLength, 0) < 102){
            gotoxy(20 - game.lastOneLength, 0);
            for(uint8_t i = 0; i < game.lastOneLength; i++){
                printf(" ");
            }
            gotoxy(19, 1);
            printf(" ");
        }
    }
    wait_vbl_done();
}

void processInputs() {
    static uint8_t longPresses = 0;
    static uint8_t input_timer = 0;

    if (input_timer > 0) {
        input_timer--;
        return;
    }

    uint8_t input = joypad();
    switch(input) {
        case J_LEFT:
            if (game.curx > 0) {
                if(game.camx > 1 && game.curx == 1){
                    game.camx--;
                } else {
                    game.curx--;
                }
            }
            input_timer = input_delay;
            break;
        case J_RIGHT:
            if (game.curx < game.width - 1) {
                if(game.camx < game.width - 20 && game.curx == 18){
                    game.camx++;
                } else {
                    game.curx++;
                }
            }
            input_timer = input_delay;
            break;
        case J_UP:
            if(game.settings == 1){
                // decrement menu cursor unitl it reaches 0 reset to 2:
                game.menuCursor--;
                if(game.menuCursor == 0){
                    game.menuCursor = 3;
                }
            } else {
                if (game.cury > 0) {
                    if(game.camy > 1 && game.cury == 1){
                        game.camy--;
                    } else {
                        game.cury--;
                    }
                }
            }

            input_timer = input_delay;
            break;
        case J_DOWN:
            if(game.settings == 1){
                game.menuCursor++;
                if(game.menuCursor == 4){
                    game.menuCursor = 1;
                }
            } else {
                if (game.cury < game.height - 1) {
                    if(game.camy < game.height - 18 && game.cury == 16){
                        game.camy++;
                    } else {
                        game.cury++;
                    }
                }
            }

            input_timer = input_delay;
            break;
        case J_A:
            if(game.settings == 1){
                if (game.menuCursor == 3){
                    resetSavedGame();
                    reset();
                } else if(game.menuCursor == 2){
                    resetSavedGame();
                    saveGame();
                } else if(game.menuCursor == 1){
                    if(game.updateState == 0){
                        game.updateState = 1;
                        game.firsTimeManu = 0;
                    } else {
                        game.updateState = 0;
                    }
                }
            } else {
                if (game.running == 0){
                    if(game.selectedCell & 0x3F == 0){
                        map[game.curx + game.camx][game.cury + game.camy] = 0;
                    } else {
                        map[game.curx + game.camx][game.cury + game.camy] = game.selectedCell;
                    }
                }
            }
            input_timer = input_delay;
            break;
        case J_B:
            if(game.running == 0){
                game.selectedCell = ((game.selectedCell & 0b00111111) | ((game.selectedCell & 0b11000000) + 0b01000000));
                input_timer = input_delay;
            }
            break;
        case J_SELECT:
            if(game.running == 0){
                game.selectedCell = (game.selectedCell & 0xC0) | ((game.selectedCell & 0x3F) + 1) % 10;
                input_timer = input_delay;
            }
            break;
        case J_START:
            if(game.settings == 1 && longPresses == 0){
                // load default font:
                disable_interrupts();
                DISPLAY_OFF;
                font_init();
                font_set(font_load(font_ibm));
                cls();
                delay(20);
                DISPLAY_ON;
                enable_interrupts();
                wait_vbl_done();
                game.running = 0;
                game.settings = 0;
                game.shouldPrintRot = 1;
                adjustInputDelay();
            } else {
                if(longPresses != 2 && game.updateState == 0){
                    if(game.running == 1){
                        game.running = 0;
                        game.shouldPrintRot = 1;
                        for(uint8_t y = 0; y < game.height; y++){
                            for(uint8_t x = 0; x < game.width; x++){
                                map[x][y] = resMap[x][y];
                            }
                        }
                    } else {
                        game.running = 1;
                        for(uint8_t y = 0; y < game.height; y++){
                            for(uint8_t x = 0; x < game.width; x++){
                                resMap[x][y] = map[x][y];
                            }
                        }
                    }
                    longPresses++;
                } else if(longPresses != 2 && game.updateState == 1 && game.settings == 0){
                    if(game.firsTimeManu == 0){
                        for(uint8_t y = 0; y < game.height; y++){
                            for(uint8_t x = 0; x < game.width; x++){
                                resMap[x][y] = map[x][y];
                            }
                        }
                        game.firsTimeManu = 1;
                    }

                    updateGame(0);
                    longPresses++;
                } else {
                    longPresses++;
                    if(game.updateState == 1 && game.firsTimeManu == 1){
                        for(uint8_t y = 0; y < game.height; y++){
                            for(uint8_t x = 0; x < game.width; x++){
                                map[x][y] = resMap[x][y];
                            }
                        }
                        game.firsTimeManu = 0;
                    }
                    game.running = 0;
                    game.settings = 1;
                    game.shouldPrintRot = 1;
                    cls();
                    set_bkg_data(0, 65, settings_data);
                    set_bkg_tiles(0, 0, 20, 18, settings_map);
                    wait_vbl_done();
                    adjustInputDelay();
                }
            }
            input_timer = input_delay;
            break;
        default:
            if(longPresses != 0) longPresses = 0;
            break;
    }
}

// 0 right
// 2 left
// 3 up
// 1 down

void updateGame(uint8_t state) {
    int x, y;
    // first 2 bits: map[x][y] & 0xC0;
    // last 6 bits: (map[x][y] & 0x3F)
    
    if(state == 0){
        for(uint8_t i = 0; i < 3; i++){
            for (y = 0; y < game.height; y++) {
                for (x = game.width - 1; x >= 0; x--) {
                    uint8_t rot = (map[x][y] & 0b11000000) >> 6;
                    uint8_t cell = (map[x][y] & 0x3F);
                    if(cell != 0 && rot == 0){
                        updateCell(x, y, i, cell, rot, 255);
                    }
                }
            }
        }
    } else if(state == 1){
        // loop from bottom to top and right to left:
        for(uint8_t i = 0; i < 3; i++){
            for (y = game.height - 1; y >= 0; y--) {
                for (x = game.width - 1; x >= 0; x--) {
                    uint8_t rot = (map[x][y] & 0b11000000) >> 6;
                    uint8_t cell = (map[x][y] & 0x3F);
                    if(cell != 0 && rot == 1){
                        updateCell(x, y, i, cell, rot, 255);
                    }
                }
            }
        }
    } else if(state == 2){
        // loop left to right and top to bottom:
        for(uint8_t i = 0; i < 3; i++){
            for (y = 0; y < game.height; y++) {
                for (x = 0; x < game.width; x++) {
                    uint8_t rot = (map[x][y] & 0b11000000) >> 6;
                    uint8_t cell = (map[x][y] & 0x3F);
                    if(cell != 0 && rot == 2){
                        updateCell(x, y, i, cell, rot, 255);
                    }
                }
            }
        }
    } else if(state == 3){
        //loop from top to bottom and left to right:
        for(uint8_t i = 0; i < 3; i++){
            for (x = 0; x < game.height; x++) {
                for (y = 0; y < game.width; y++) {
                    uint8_t rot = (map[x][y] & 0b11000000) >> 6;
                    uint8_t cell = (map[x][y] & 0x3F);
                    if(cell != 0 && rot == 3){
                        updateCell(x, y, i, cell, rot, 255);
                    }
                }
            }
        }
    }

    if(state < 3){
        updateGame(state + 1);
    }
}

void updateCell(int x, int y, uint8_t cellI, uint8_t id, uint8_t rot, uint8_t state){
    if (cellI == 0 && id == 2) {
        if(rot == 0){
            // check if the back of the generator is not empty or the border of the game:
            if(x - 1 >= 0 && x + 1 < game.width){
                uint8_t genCell = map[x - 1][y] & 0x3F;
                uint8_t genRot = (map[x - 1][y] & 0xC0) >> 6;
                uint8_t frontCell = map[x + 1][y] & 0x3F;
                uint8_t frontRot = (map[x + 1][y] & 0xC0) >> 6;

                if(genCell != 0){
                    if(frontCell != 8 && frontCell != 7){
                        if(frontCell == 0){
                            map[x + 1][y] = map[x - 1][y];
                        } else if(frontCell == 9){
                            map[x + 1][y] = 0;
                        } else if(pushCell(x + 1, y, rot, 0) == 1){
                            map[x + 1][y] = map[x - 1][y];
                        }
                    }
                }
            }
        } else if(rot == 1){
            if(y - 1 >= 0 && y + 1 < game.height){
                uint8_t genCell = map[x][y - 1] & 0x3F;
                uint8_t genRot = (map[x][y - 1] & 0xC0) >> 6;
                uint8_t frontCell = map[x][y + 1] & 0x3F;
                uint8_t frontRot = (map[x][y + 1] & 0xC0) >> 6;

                if(genCell != 0){
                    if(frontCell != 8 && frontCell != 7){
                        if(frontCell == 0){
                            map[x][y + 1] = map[x][y - 1];
                        } else if(frontCell == 9){
                            map[x][y + 1] = 0;
                        } else if(pushCell(x, y + 1, rot, 0) == 1){
                            map[x][y + 1] = map[x][y - 1];
                        }
                    }
                }
            }
        } else if(rot == 2){
            if(x - 1 >= 0 && x + 1 < game.width){
                uint8_t genCell = map[x + 1][y] & 0x3F;
                uint8_t genRot = (map[x + 1][y] & 0xC0) >> 6;
                uint8_t frontCell = map[x - 1][y] & 0x3F;
                uint8_t frontRot = (map[x - 1][y] & 0xC0) >> 6;

                if(genCell != 0){
                    if(frontCell != 8 && frontCell != 7){
                        if(frontCell == 0){
                            map[x - 1][y] = map[x + 1][y];
                        } else if(frontCell == 9){
                            map[x - 1][y] = 0;
                        } else if(pushCell(x - 1, y, rot, 0) == 1){
                            map[x - 1][y] = map[x + 1][y];
                        }
                    }
                }
            }
        } else if(rot == 3){
            if(y - 1 >= 0 && y + 1 < game.height){
                uint8_t genCell = map[x][y + 1] & 0x3F;
                uint8_t genRot = (map[x][y + 1] & 0xC0) >> 6;
                uint8_t frontCell = map[x][y - 1] & 0x3F;
                uint8_t frontRot = (map[x][y - 1] & 0xC0) >> 6;

                if(genCell != 0){
                    if(frontCell != 8 && frontCell != 7){
                        if(frontCell == 0){
                            map[x][y - 1] = map[x][y + 1];
                        } else if(frontCell == 9){
                            map[x][y - 1] = 0;
                        } else if(pushCell(x, y - 1, rot, 0) == 1){
                            map[x][y - 1] = map[x][y + 1];
                        }
                    }
                }
            }
        }
    } else

    if (cellI == 1 && (id == 3 || id == 4)) {
        // rotators:
        if(id == 3) {
            // CW:
            if(x - 1 >= 0 && map[x - 1][y] != 0)              map[x - 1][y] = ((map[x - 1][y] & 0b00111111) | ((map[x - 1][y] & 0b11000000) + 0b01000000));
            if(x + 1 < game.width && map[x + 1][y] != 0)      map[x + 1][y] = ((map[x + 1][y] & 0b00111111) | ((map[x + 1][y] & 0b11000000) + 0b01000000));
            if(y - 1 >= 0 && map[x][y - 1] != 0)              map[x][y - 1] = ((map[x][y - 1] & 0b00111111) | ((map[x][y - 1] & 0b11000000) + 0b01000000));
            if(y + 1 < game.height && map[x][y + 1] != 0)     map[x][y + 1] = ((map[x][y + 1] & 0b00111111) | ((map[x][y + 1] & 0b11000000) + 0b01000000));
        } else {
            // ACW:
            if(x - 1 >= 0 && map[x - 1][y] != 0)              map[x - 1][y] = ((map[x - 1][y] & 0b00111111) | ((map[x - 1][y] & 0b11000000) - 0b01000000));
            if(x + 1 < game.width && map[x + 1][y] != 0)      map[x + 1][y] = ((map[x + 1][y] & 0b00111111) | ((map[x + 1][y] & 0b11000000) - 0b01000000));
            if(y - 1 >= 0 && map[x][y - 1] != 0)              map[x][y - 1] = ((map[x][y - 1] & 0b00111111) | ((map[x][y - 1] & 0b11000000) - 0b01000000));
            if(y + 1 < game.height && map[x][y + 1] != 0)     map[x][y + 1] = ((map[x][y + 1] & 0b00111111) | ((map[x][y + 1] & 0b11000000) - 0b01000000));
        }
    } else

    if (cellI == 2 && id == 1) {
        // movers:
        if(rot == 0){
            // right
            if(x + 1 <= game.width - 1){
                if(map[x + 1][y] != 0){
                    if(map[x + 1][y] == 7){
                        map[x][y] = 0;
                    } else if(map[x + 1][y] == 9){
                        map[x][y] = 0;
                        map[x + 1][y] = 0;
                    } else if(pushCell(x + 1, y, rot, 1) == 1){
                        map[x + 1][y] = map[x][y];
                        map[x][y] = 0;
                    }
                } else {
                    map[x + 1][y] = map[x][y];
                    map[x][y] = 0;
                }
            }
        } else if(rot == 1){
            // down
            if(y + 1 <= game.height - 1) {
                if(map[x][y + 1] != 0){
                    if(map[x][y + 1] == 7){
                        map[x][y] = 0;
                    } else if(map[x][y + 1] == 9){
                        map[x][y] = 0;
                        map[x][y + 1] = 0;
                    } else if(pushCell(x, y + 1, rot, 1) == 1){
                        map[x][y + 1] = map[x][y];
                        map[x][y] = 0;
                    }
                } else {
                    map[x][y + 1] = map[x][y];
                    map[x][y] = 0;
                }
            }
        } else if(rot == 2){
            // left
            if(x - 1 >= 0){
                if(map[x - 1][y] != 0){
                    if(map[x - 1][y] == 7){
                        map[x][y] = 0;
                    } else if(map[x - 1][y] == 9) {
                        map[x][y] = 0;
                        map[x - 1][y] = 0;
                    } else if(pushCell(x - 1, y, rot, 1) == 1){
                        map[x - 1][y] = map[x][y];
                        map[x][y] = 0;
                    }
                } else {
                    map[x - 1][y] = map[x][y];
                    map[x][y] = 0;
                }
            }
        } else if(rot == 3){
            // up
            if(y - 1 >= 0){
                if(map[x][y - 1] != 0){
                    if(map[x][y - 1] == 7){
                        map[x][y] = 0;
                    } else if(map[x][y - 1] == 9){
                        map[x][y] = 0;
                        map[x][y - 1] = 0;
                    } else if(pushCell(x, y - 1, rot, 1) == 1){
                        map[x][y - 1] = map[x][y];
                        map[x][y] = 0;
                    }
                } else {
                    map[x][y - 1] = map[x][y];
                    map[x][y] = 0;
                }
            }
        }
    }
}



uint8_t pushCell(int x, int y, uint8_t rot, uint8_t isMover){
    uint8_t state = 0;
    uint8_t totalMoverCount = 0; // todo:
    uint8_t totalBadMoverCount = 0; // todo:
    // reach the unpushable or the trash cell and record the distance long:
    int distance = 0;

    if(isMover == 1){
        totalMoverCount++;
    }

    if(rot == 0){
        while(1){
            if(x + distance > game.width - 1){
                state = 1;
                break;
            }

            uint8_t cell = map[x + distance][y] & 0x3F;
            uint8_t cRot = (map[x + distance][y] & 0xC0) >> 6;
            
            if(cell == 0){
                break;
            } else if(cell == 1){
                if(abs(cRot - rot) == 2){
                    totalBadMoverCount++;
                } else if(cRot == rot){
                    totalMoverCount++;
                }
                distance++;
            } else if((cell == 5 && isPerpendicurlar(cRot, rot) == 1) || cell == 8){
                state = 1;
                break;
            } else if(cell == 7){
                state = 2;
                break;
            } else if(cell == 9){
                state = 3;
                break;
            } else {
                distance++;
            }
        }
        
        if(!(totalMoverCount > totalBadMoverCount) && isMover == 1){
            return 0;
        } else if(!(totalMoverCount >= totalBadMoverCount) && isMover != 1){
            return 0;
        }
            
        if(state == 0){
            if(distance == 0){
                return 1;
            }
            for(uint8_t i = distance; i > 0; i--){
                map[x + i][y] = map[x + i - 1][y];
                map[x + i - 1][y] = 0;
            }
            return 1;
        } else if(state == 1){
            return 0;
        } else if(state == 2 || state == 3){
            map[x + distance - 1][y] = 0;
            if(state == 3){
                map[x + distance][y] = 0;
            }

            // push the remaining cell:
            for(uint8_t i = distance - 1; i > 0; i--){
                map[x + i][y] = map[x + i - 1][y];
                map[x + i - 1][y] = 0;
            }
            return 1;
        }
    } else if(rot == 1) {
        // down
        while(1){
            if(y + distance > game.height - 1){
                state = 1;
                break;
            }

            uint8_t cell = map[x][y + distance] & 0x3F;
            uint8_t cRot = (map[x][y + distance] & 0xC0) >> 6;
            
            if(cell == 0){
                break;
            } else if(cell == 1){
                if(abs(cRot - rot) == 2){
                    totalBadMoverCount++;
                } else if(cRot == rot){
                    totalMoverCount++;
                }
                distance++;
            } else if((cell == 5 && isPerpendicurlar(cRot, rot) == 1) || cell == 8){
                state = 1;
                break;
            } else if(cell == 7){
                state = 2;
                break;
            } else if(cell == 9){
                state = 3;
                break;
            } else {
                distance++;
            }
        }

        if(!(totalMoverCount > totalBadMoverCount) && isMover == 1){
            return 0;
        } else if(!(totalMoverCount >= totalBadMoverCount) && isMover != 1){
            return 0;
        }

        if(state == 0){
            if(distance == 0){
                return 1;
            }
            // push the cell:
            for(uint8_t i = distance; i > 0; i--){
                map[x][y + i] = map[x][y + i - 1];
                map[x][y + i - 1] = 0;
            }
            return 1;
        } else if(state == 1){
            return 0;
        } else if(state == 2 || state == 3){
            map[x][y + distance - 1] = 0;
            if(state == 3){
                map[x][y + distance] = 0;
            }

            // push the remaining cell:
            for(uint8_t i = distance - 1; i > 0; i--){
                map[x][y + i] = map[x][y + i - 1];
                map[x][y + i - 1] = 0;
            }
            return 1;
        }
    } else if(rot == 2){
        // left
        while(1){
            if(x - distance < 0){
                state = 1;
                break;
            }

            uint8_t cell = map[x - distance][y] & 0x3F;
            uint8_t cRot = (map[x - distance][y] & 0xC0) >> 6;
            
            if(cell == 0){
                break;
            } else if(cell == 1){
                if(abs(cRot - rot) == 2){
                    totalBadMoverCount++;
                } else if(cRot == rot){
                    totalMoverCount++;
                }
                distance++;
            } else if((cell == 5 && isPerpendicurlar(cRot, rot) == 1) || cell == 8){
                state = 1;
                break;
            } else if(cell == 7){
                state = 2;
                break;
            } else if(cell == 9){
                state = 3;
                break;
            } else {
                distance++;
            }
        }

        if(!(totalMoverCount > totalBadMoverCount) && isMover == 1){
            return 0;
        } else if(!(totalMoverCount >= totalBadMoverCount) && isMover != 1){
            return 0;
        }

        if(state == 0){
            if(distance == 0){
                return 1;
            }   
            // push the cell:
            for(uint8_t i = distance; i > 0; i--){
                map[x - i][y] = map[x - i + 1][y];
                map[x - i + 1][y] = 0;
            }
            return 1;
        } else if(state == 1){
            return 0;
        } else if(state == 2 || state == 3){
            map[x - distance + 1][y] = 0;
            if(state == 3){
                map[x - distance][y] = 0;
            }

            // push the remaining cell:
            for(uint8_t i = distance - 1; i > 0; i--){
                map[x - i][y] = map[x - i + 1][y];
                map[x - i + 1][y] = 0;
            }
            return 1;
        }
    } else if(rot == 3){
        // up
        while(1){
            if(y - distance < 0){
                state = 1;
                break;
            }

            uint8_t cell = map[x][y - distance] & 0x3F;
            uint8_t cRot = (map[x][y - distance] & 0xC0) >> 6;
            
            if(cell == 0){
                break;
            } else if(cell == 1){
                if(abs(cRot - rot) == 2){
                    totalBadMoverCount++;
                } else if(cRot == rot){
                    totalMoverCount++;
                }
                distance++;
            } else if((cell == 5 && isPerpendicurlar(cRot, rot) == 1) || cell == 8){
                state = 1;
                break;
            } else if(cell == 7){
                state = 2;
                break;
            } else if(cell == 9){
                state = 3;
                break;
            } else {
                distance++;
            }
        }

        if(!(totalMoverCount > totalBadMoverCount) && isMover == 1){
            return 0;
        } else if(!(totalMoverCount >= totalBadMoverCount) && isMover != 1){
            return 0;
        }

        if(state == 0){
            if(distance == 0){
                return 1;
            }
            // push the cell:
            for(uint8_t i = distance; i > 0; i--){
                map[x][y - i] = map[x][y - i + 1];
                map[x][y - i + 1] = 0;
            }
            return 1;
        } else if(state == 1){
            return 0;
        } else if(state == 2 || state == 3){
            map[x][y - distance + 1] = 0;
            if(state == 3){
                map[x][y - distance] = 0;
            }

            // push the remaining cell:
            for(uint8_t i = distance - 1; i > 0; i--){
                map[x][y - i] = map[x][y - i + 1];
                map[x][y - i + 1] = 0;
            }
            return 1;
        }
    }
    
    return 0;
}

void saveGame(){
    // write map to savedGame array mono-dimensional:
    // ENABLE_RAM_MBC1;
    fingerPrint[0] = 0x32;
    fingerPrint[1] = 0xA4;
    fingerPrint[2] = 0x8F;
    savedWidth = game.width;
    savedHeight = game.height;
    for(uint8_t y = 0; y < game.height; y++){
        for(uint8_t x = 0; x < game.width; x++){
            savedGame[x][y] = map[x][y];
        }
    }
    // DISABLE_RAM_MBC1;
}

void resetSavedGame(){
    // reset the savedGame array:
    // ENABLE_RAM_MBC1;
    fingerPrint[0] = 0x69;
    for(uint8_t y = 0; y < 50; y++){
        for(uint8_t x = 0; x < 50; x++){
            savedGame[x][y] = 0;
        }
    }
    // DISABLE_RAM_MBC1;
}

void loadSavedGame(){
    // get the width and height of the game:
    // ENABLE_RAM_MBC1;
    game.width = savedWidth; 
    game.height = savedHeight;
    for(uint8_t y = 0; y < game.height; y++){
        for(uint8_t x = 0; x < game.width; x++){
            map[x][y] = savedGame[x][y];
        }
    }
    // DISABLE_RAM_MBC1;
}

uint8_t checkIfSavedMap(){
    //  ENABLE_RAM_MBC1;
    if(fingerPrint[0] == 0x32 && fingerPrint[1] == 0xA4 && fingerPrint[2] == 0x8F){
        return 1;
    } else {
        return 0;
    }
    // DISABLE_RAM_MBC1;
}

uint8_t isPerpendicurlar(uint8_t rot1, uint8_t rot2){
    // NOTE: maybe swtich to int for better performance(will let the usage of absulute values)

    if(rot1 == rot2){
        // optmizition purposes
        return 0;
    } else if(rot1 == 0 && rot2 == 1){
        return 1;
    } else if(rot1 == 1 && rot2 == 0){
        return 1;
    } else if(rot1 == 2 && rot2 == 3){
        return 1;
    } else if(rot1 == 3 && rot2 == 2){
        return 1;
    } else if(rot1 == 0 && rot2 == 3){
        return 1;
    } else if(rot1 == 3 && rot2 == 0){
        return 1;
    } else if(rot1 == 1 && rot2 == 2){
        return 1;
    } else if(rot1 == 2 && rot2 == 1) {
        return 1;
    } else {
        return 0;
    }
}

void adjustInputDelay(){
    if(game.settings == 0){
        if(game.width * game.height > 400){
            input_delay = 0;
        } else if(game.width * game.height > 200){
            input_delay = 1;
        }
    } else {
        input_delay = 2;
    }
}