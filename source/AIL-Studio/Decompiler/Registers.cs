namespace AIL_Studio.Decompiler
{
    /// <summary>Maps Artemis-IL register identifier bytes back to their names.</summary>
    internal static class Registers
    {
        public static string GetName(byte b) => b switch
        {
            0xF0 => "PC",
            0xF1 => "IP",
            0xF2 => "SP",
            0xF3 => "SS",
            0xF4 => "A",
            0xF5 => "AL",
            0xF6 => "AH",
            0xF7 => "B",
            0xF8 => "BL",
            0xF9 => "BH",
            0xFA => "C",
            0xFB => "CL",
            0xFC => "CH",
            0xFD => "X",
            0xFE => "Y",
            _    => "",
        };

        public static bool IsRegister(byte b) => b >= 0xF0;
    }
}
