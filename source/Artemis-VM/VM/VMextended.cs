using System;
using Artemis_IL.StandardLib;
using Artemis_IL.Conversions;

namespace Artemis_IL
{
    public partial class VM
    {
        /// <summary>
        /// Reads a little-endian 32-bit integer from RAM starting at startingIndex.
        /// </summary>
        /// <param name="startingIndex">Starting byte offset in RAM</param>
        /// <returns>32-bit integer value</returns>
        private int Get32BitParameter(int startingIndex)
        {
            return ram.memory[startingIndex]
                 | (ram.memory[startingIndex + 1] << 8)
                 | (ram.memory[startingIndex + 2] << 16)
                 | (ram.memory[startingIndex + 3] << 24);
        }

        /// <summary>
        /// Places content into a register, splitting if necessary
        /// </summary>
        /// <param name="Register"></param>
        /// <param name="Content"></param>
        private void SetRegister(byte Register, int Content)
        {
            if (Register == (byte)0xF0)
                PC = (byte)Content;
            else if (Register == (byte)0xF1)
                IP = (byte)Content;
            //0xF2 (Stack Pointer) is read only
            else if (Register == (byte)0xF3)
                SS = (byte)Content;
            else if (Register == (byte)0xF4)
                SetSplit('A', Content);
            else if (Register == (byte)0xF5)
                AL = (byte)Content;
            else if (Register == (byte)0xF6)
                AH = (byte)Content;
            else if (Register == (byte)0xF7)
                SetSplit('B', Content);
            else if (Register == (byte)0xF8)
                BL = (byte)Content;
            else if (Register == (byte)0xF9)
                BH = (byte)Content;
            else if (Register == (byte)0xFA)
                SetSplit('C', Content);
            else if (Register == (byte)0xFB)
                CL = (byte)Content;
            else if (Register == (byte)0xFC)
                CH = (byte)Content;
            else if (Register == (byte)0xFD)
                X = Content;
            else if (Register == (byte)0xFE)
                Y = Content;
            else
                throw new Exception("ERROR: The register " + Register + " is not a register.");
        }

        /// <summary>
        /// Returns an integer value stored in a register
        /// </summary>
        /// <param name="Reg"></param>
        /// <returns></returns>
        private int GetRegister(byte Register)
        {
            if (Register == (byte)0xF0)
                return (int)PC;
            else if (Register == (byte)0xF1)
                return (int)IP;
            else if (Register == (byte)0xF2)
                return (int)SP;
            else if (Register == (byte)0xF3)
                return (int)SS;
            else if (Register == (byte)0xF4)
                return GetSplit('A');
            else if (Register == (byte)0xF5)
                return AL;
            else if (Register == (byte)0xF6)
                return AH;
            else if (Register == (byte)0xF7)
                return GetSplit('B');
            else if (Register == (byte)0xF8)
                return BL;
            else if (Register == (byte)0xF9)
                return BH;
            else if (Register == (byte)0xFA)
                return GetSplit('C');
            else if (Register == (byte)0xFB)
                return CL;
            else if (Register == (byte)0xFC)
                return CH;
            else if (Register == (byte)0xFD)
                return X;
            else if (Register == (byte)0xFE)
                return Y;
            else
                return 0;
        }
    }
}