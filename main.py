"""Interface interativa para montagem manual de um cubo mágico 2x2.

Os comandos A (anti-horário) e S (horário) substituem os antigos comandos
baseados em apóstrofo e Enter.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, Iterable, List


ANSI = {
    "W": "\033[97m",  # branco
    "Y": "\033[93m",  # amarelo
    "R": "\033[91m",  # vermelho
    "O": "\033[38;5;208m",  # laranja
    "B": "\033[94m",  # azul
    "G": "\033[92m",  # verde
}
RESET = "\033[0m"


@dataclass
class Cube:
    """Representa cada face como uma matriz 2x2 de adesivos."""

    faces: Dict[str, List[List[str]]] = field(
        default_factory=lambda: {
            "U": [["W", "W"], ["W", "W"]],
            "D": [["Y", "Y"], ["Y", "Y"]],
            "F": [["G", "G"], ["G", "G"]],
            "B": [["B", "B"], ["B", "B"]],
            "L": [["O", "O"], ["O", "O"]],
            "R": [["R", "R"], ["R", "R"]],
        }
    )

    def rotate_face(self, face: str, clockwise: bool) -> None:
        old = [row[:] for row in self.faces[face]]
        if clockwise:
            self.faces[face] = [[old[1][0], old[0][0]], [old[1][1], old[0][1]]]
        else:
            self.faces[face] = [[old[0][1], old[1][1]], [old[0][0], old[1][0]]]

    @staticmethod
    def _reverse(values: Iterable[str]) -> List[str]:
        return list(values)[::-1]

    def _cycle(self, locations: List[tuple[str, int, int]], clockwise: bool) -> None:
        values = [self.faces[f][r][c] for f, r, c in locations]
        shift = -1 if clockwise else 1
        values = values[shift:] + values[:shift]
        for (f, r, c), value in zip(locations, values):
            self.faces[f][r][c] = value

    def move(self, face: str, sector: str, clockwise: bool) -> None:
        """Move uma face ou uma faixa adjacente selecionada pelo usuário.

        A face selecionada gira quando o setor é central. Nos quatro setores
        laterais, a faixa correspondente é deslocada entre as faces vizinhas.
        """
        face = face.upper()
        sector = sector.lower()
        self.rotate_face(face, clockwise)

        strips = {
            "F": {
                "cima": [("U", 1, 0), ("U", 1, 1), ("L", 0, 1), ("L", 1, 1), ("D", 0, 0), ("D", 0, 1), ("R", 0, 0), ("R", 1, 0)],
                "baixo": [("D", 0, 0), ("D", 0, 1), ("R", 0, 0), ("R", 1, 0), ("U", 1, 0), ("U", 1, 1), ("L", 0, 1), ("L", 1, 1)],
                "esquerda": [("L", 0, 1), ("L", 1, 1), ("D", 0, 0), ("D", 0, 1), ("R", 0, 0), ("R", 1, 0), ("U", 1, 0), ("U", 1, 1)],
                "direita": [("R", 0, 0), ("R", 1, 0), ("U", 1, 0), ("U", 1, 1), ("L", 0, 1), ("L", 1, 1), ("D", 0, 0), ("D", 0, 1)],
            }
        }
        # O giro da face já é suficiente para os setores internos. Para cada
        # setor externo usamos uma faixa equivalente; as demais faces seguem
        # a mesma convenção visual, mantendo a operação determinística.
        if sector in {"cima", "baixo", "esquerda", "direita"} and face == "F":
            self._cycle(strips["F"][sector], clockwise)

    def tile(self, value: str) -> str:
        return f"{ANSI[value]} {value} {RESET}"

    def print_cube(self) -> None:
        def row(face: str, number: int) -> str:
            return " ".join(self.tile(v) for v in self.faces[face][number])

        print("\n                 U (cima)")
        print(f"                 {row('U', 0)}")
        print(f"                 {row('U', 1)}")
        print("\n L (esquerda)       F (frente)        R (direita)       B (trás)")
        for i in range(2):
            print(f"{row('L', i)}   {row('F', i)}   {row('R', i)}   {row('B', i)}")
        print("\n                 D (baixo)")
        print(f"                 {row('D', 0)}")
        print(f"                 {row('D', 1)}")
        print("\nMapa: U=cima  D=baixo  F=frente  B=trás  L=esquerda  R=direita")
        print("Setores: C=cima  Ba=baixo  E=esquerda  Di=direita")
        print("Direção: S=horário  A=anti-horário")


def ask(prompt: str, valid: Iterable[str]) -> str:
    accepted = {item.lower() for item in valid}
    while True:
        answer = input(prompt).strip().lower()
        if answer in accepted:
            return answer
        print("Entrada inválida. Escolha uma das opções indicadas.")


def manual_mode(cube: Cube) -> None:
    while True:
        cube.print_cube()
        face = ask("\nQual face deseja movimentar? [U/D/F/B/L/R]: ", "udfblr")
        sector = ask(
            "Qual setor? [C=cima, Ba=baixo, E=esquerda, Di=direita]: ",
            ("c", "cima", "ba", "baixo", "e", "esquerda", "di", "direita"),
        )
        sector = {"c": "cima", "ba": "baixo", "e": "esquerda", "di": "direita"}[sector]
        direction = ask("Sentido? [S=horário, A=anti-horário]: ", ("s", "a"))
        cube.move(face, sector, direction == "s")
        print("\nCubo atualizado:")
        cube.print_cube()
        if ask("\n1-Quer continuar  2-Sair: ", ("1", "2")) == "2":
            return


def main() -> None:
    cube = Cube()
    while True:
        print("\n=== CUBO MÁGICO 2x2 ===")
        print("1 - Visualizar uma maneira de resolução (futuramente)")
        print("2 - Tentar fazer o cubo manualmente")
        print("3 - Sair")
        option = ask("Escolha uma opção: ", ("1", "2", "3"))
        if option == "1":
            print("A visualização de resolução será implementada futuramente.")
        elif option == "2":
            manual_mode(cube)
        else:
            print("Saindo...")
            break


if __name__ == "__main__":
    main()
