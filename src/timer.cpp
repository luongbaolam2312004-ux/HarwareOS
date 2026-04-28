
class Counter
{
protected:
    int _value;
public:
    Counter()
    {
        _value = 0; 
    }

    void Start(int interval)
    {
        _value = interval;
    }

    void Stop()
    {
        _value = 0;
    }

    void Tick()
    {
        if(_value > 0)
        {
            _value--;   //Count
            on_tick(); //Continue counting

            if(_value == 0)
            {
                on_interrupt();//Start interrupt
            }
        }
    }

protected:
    virtual void on_tick(){}
    virtual void on_interrupt();
};

class Timer: Counter
{
    int _interval;
public:
    Timer(int interval)
    {
        _interval = interval;
    }

    void Start()
    {
        Counter::Start(_interval);
    }
    void Stop()
    {
        Counter::Stop();
    }
protected:
    void on_interrupt() override
    {
        Counter::Start(_interval);
        on_reset();
    }
    virtual void on_reset()
    {

    }
};