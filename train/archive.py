
from bitstring import ConstBitStream, ReadError
import random
import numpy as np

BOARD_SIZE = 19
BOARD_SQ = BOARD_SIZE*BOARD_SIZE
HISTORY_STEP = 8
INPUT_CHANNELS = 2*HISTORY_STEP + 2

RESIDUAL_FILTERS = 192
RESIDUAL_BLOCKS = 13


class GameSteps(object):
    def __init__(self):
        self.steps = []
        self.valid_steps = []
        self.result = 0

    def add(self, valid, pos, removes, probs):
        index = len(self.steps)
        self.steps.append((pos, tuple(removes), tuple(probs)))
        if valid:
            self.valid_steps.append(index)

    def encode(self, index):
        
        if index%2 == 0:
            player_is_black = True
        else:
            player_is_black = False

        blacks = [0]*BOARD_SQ
        whites = [0]*BOARD_SQ
        black_history = []
        white_history = []
        probs = []
        planes = [None]*(HISTORY_STEP*2)

        black_move = True
        for step in range(index):
            pos = self.steps[step][0]
            if pos < BOARD_SQ:
                if black_move:
                    blacks[pos] = 1
                else:
                    whites[pos] = 0

            # remove stones
            if self.steps[step][1]:
                for rmpos in self.steps[step][1]:
                    blacks[rmpos] = 0
                    whites[rmpos] = 0

            h = index - step - 1
            if h < HISTORY_STEP:
                if player_is_black:
                    planes[h] = np.array(blacks, dtype=np.uint8)
                    planes[HISTORY_STEP + h] = np.array(whites, dtype=np.uint8)
                else:
                    planes[h] = np.array(whites, dtype=np.uint8)
                    planes[HISTORY_STEP + h] = np.array(blacks, dtype=np.uint8)

            black_move = not black_move

        for i in range(HISTORY_STEP*2):
            if planes[i] is None:
                planes[i] = np.array([0]*BOARD_SQ, dtype=np.uint8)

        
        probs = self.steps[index][2]
        if len(probs) == 0:
            probs = [0] * (BOARD_SQ+1)
            probs[self.steps[index][0]] = 1

        probabilities = np.array(probs).astype(float)

        result = self.result
        if not player_is_black:
            result = -result

        return planes, probabilities, player_is_black, result


class GameArchive(object):
    def __init__(self, path):

        self.games = []

        with open(path, 'rb') as file:
            raw_data = ConstBitStream(file)
            type = raw_data.read('int:8')
            if type != ord('G'):
                raise Exception("bad archive type magic")
            bsize = raw_data.read('int:8')
            if bsize != BOARD_SIZE:
                raise Exception("not expect traindata for BOARD_SIZE %d" % (BOARD_SIZE,))

            while True:
                try:
                    magic = raw_data.read('int:8')
                    if magic != ord('g'):
                        raise Exception("bad archive line magic")
                except ReadError:
                    break # EOF

                self.parse_game_line(raw_data)

 
    def parse_game_line(self, raw_data):

        result = raw_data.read('int:8')
        steps = raw_data.read('uintle:16')

        if result not in (0, 1, -1):
            raise Exception("bad game result")

        game_index = len(self.games)
        game_steps = GameSteps()

        for step in range(steps):
            move = raw_data.read('uintle:16')
            pos = 0x1ff & move

            if pos > BOARD_SQ:
                raise Exception("invalid move pos %d" % pos)

            probs = []
            removes  = []
            # remove stones
            if move & 0x8000:
                rm = raw_data.read('uintle:16')
                for _ in range(rm):
                    rmpos = raw_data.read('uintle:16')
                    if rmpos >= BOARD_SQ:
                        raise Exception("invalid remove pos %d" % rmpos)
                    removes.append(rmpos)

            if move & 0x4000:
                probs = raw_data.read(['floatle:32']* (BOARD_SQ+1))

            if move & 0x2000:
                valid = False
            else:
                valid = True

            game_steps.add(valid, pos, removes, probs)

        game_steps.result = result
        self.games.append(game_steps)
                



if __name__ == "__main__":
    obj = GameArchive('../build/selfplay.data')
    obj.games[0].encode(0)

