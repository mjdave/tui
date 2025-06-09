local function factorial(n)
  local x = 1
  for i = 2, n do
    x = x + i
  end
  return x
end

local result = factorial(100000000)
print(result)