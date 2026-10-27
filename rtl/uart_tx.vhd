library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity uart_tx is
    port (
        clk          : in  std_logic;
        rst          : in  std_logic;
        baud_tick    : in  std_logic;
        start_packet : in  std_logic;
        accel_in     : in  std_logic_vector(47 downto 0);
        tx_pin       : out std_logic;
        tx_busy      : out std_logic
    );
end entity uart_tx;

architecture rtl of uart_tx is
    type frame_array_t is array (0 to 8) of std_logic_vector(7 downto 0);
    signal packet    : frame_array_t := (others => x"00");
    signal byte_idx  : integer range 0 to 8 := 0;
    signal bit_pos   : integer range 0 to 9 := 0;
    signal shreg     : std_logic_vector(9 downto 0) := (others => '1');
    signal active    : std_logic := '0';
begin
    tx_pin  <= shreg(0) when active = '1' else '1';
    tx_busy <= active;

    process(clk)
        variable b2, b3, b4, b5, b6, b7 : std_logic_vector(7 downto 0);
    begin
        if rising_edge(clk) then
            if rst = '1' then
                active   <= '0';
                shreg    <= (others => '1');
                byte_idx <= 0;
                bit_pos  <= 0;
            elsif active = '0' then
                if start_packet = '1' then
                    b2 := accel_in(47 downto 40);
                    b3 := accel_in(39 downto 32);
                    b4 := accel_in(31 downto 24);
                    b5 := accel_in(23 downto 16);
                    b6 := accel_in(15 downto 8);
                    b7 := accel_in(7 downto 0);

                    packet(0) <= x"AA";
                    packet(1) <= x"55";
                    packet(2) <= b2;
                    packet(3) <= b3;
                    packet(4) <= b4;
                    packet(5) <= b5;
                    packet(6) <= b6;
                    packet(7) <= b7;
                    packet(8) <= b2 xor b3 xor b4 xor b5 xor b6 xor b7;

                    -- Load first byte (Start bit '0' + 8 data bits + Stop bit '1')
                    shreg    <= '1' & x"AA" & '0';
                    byte_idx <= 0;
                    bit_pos  <= 0;
                    active   <= '1';
                end if;
            elsif baud_tick = '1' then
                if bit_pos = 9 then
                    bit_pos <= 0;
                    if byte_idx = 8 then
                        active <= '0';
                    else
                        byte_idx <= byte_idx + 1;
                        shreg    <= '1' & packet(byte_idx + 1) & '0';
                    end if;
                else
                    shreg   <= '1' & shreg(9 downto 1);
                    bit_pos <= bit_pos + 1;
                end if;
            end if;
        end if;
    end process;
end architecture rtl;