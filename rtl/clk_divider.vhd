library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity clk_divider is
    port (
        clk          : in  std_logic;
        rst          : in  std_logic;
        i2c_tick_400k: out std_logic;
        uart_tick_115k: out std_logic
    );
end entity clk_divider;

architecture rtl of clk_divider is
    -- 12 MHz / 30 = 400 kHz tick for I2C FSM
    signal i2c_cnt  : unsigned(4 downto 0) := (others => '0');
    -- 12 MHz / 104 = 115.384 kHz tick for 115200 baud UART
    signal uart_cnt : unsigned(6 downto 0) := (others => '0');
begin
    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                i2c_cnt        <= (others => '0');
                uart_cnt       <= (others => '0');
                i2c_tick_400k  <= '0';
                uart_tick_115k <= '0';
            else
                -- 400 kHz Divider
                if i2c_cnt = to_unsigned(29, 5) then
                    i2c_cnt       <= (others => '0');
                    i2c_tick_400k <= '1';
                else
                    i2c_cnt       <= i2c_cnt + 1;
                    i2c_tick_400k <= '0';
                end if;

                -- 115200 Baud Divider
                if uart_cnt = to_unsigned(103, 7) then
                    uart_cnt       <= (others => '0');
                    uart_tick_115k <= '1';
                else
                    uart_cnt       <= uart_cnt + 1;
                    uart_tick_115k <= '0';
                end if;
            end if;
        end if;
    end process;
end architecture rtl;