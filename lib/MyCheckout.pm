package MyCheckout;

use Moose;
use MooseX::Types::Moose qw(Str Int ArrayRef);
use namespace::autoclean;
use Data::Dumper;

has 'user'        => ( is => 'ro', required => 1, weak_ref => 1 );
has 'session'     => ( is => 'ro', required => 1, weak_ref => 1 );
has 'session_id'  => ( is => 'ro', required => 1, weak_ref => 1 );
has 'schema'      => ( is => 'rw', required => 1, handles => [qw / resultset /] );

# process order
sub process_order {
    my $self = shift;

    #process_payment($self);
    convert_cart_to_order($self);

}

sub process_payment {
    my $self = shift;
    my $cart;

    # Retrieve our cart
    $cart = $self->resultset('Cart')->get_cart_by_customer_id($self->user->id);

    if ($cart->payment_type eq 'PayPal') {

    my $paypal = Business::PayPal->new;
    my $button = $paypal->button(
        business        => 'info@saborespanol.com',
        item_name       => 'Saborespanol.com - Shopping Cart',
        item_number     => '12322',
        invoice         => '12322',
        image_url       => 'http://saborespanol.com:3000/static/images/sabor-espanol-logo.gif',
        return          => 'http://saborespanol.com:3000/checkout/receipt',
        cancel_return   => 'http://saborespanol.com:3000/checkout/payment',
        amount          => '0.01',
        quantity        => 1,
        notify_url      => 'http://www.cansecwest.com/ipn.cgi',
    );
    my $id = $paypal->id;


    } elsif ($cart->payment_type eq 'CC') {



    }

}

# convert cart to order
sub convert_cart_to_order {
    my $self = shift;
    my $customer;
    my %customer_columns;
    my $cart;
    my $cart_items;
    my %cart_columns;
    my %order_args;
    my $order;
    my $order_id;
    my %order_items;

    # Get our customer
    $customer
        = $self->resultset('Customer')->get_customer_by_id($self->user->id);

    # Map our customer columns to the order arguments
    %customer_columns = $customer->get_columns;
    foreach my $key (keys %customer_columns) {
        $order_args{$key} = $customer_columns{$key};
    }

    # Retrieve our cart
    $cart = $self->resultset('Cart')->get_cart_by_customer_id($self->user->id);

    %cart_columns = $cart->get_columns;
    foreach my $key (keys %cart_columns) {
        next if ($key eq 'session_id');
        $order_args{$key} = $cart_columns{$key};
    }

    # Fixing
    $order_args{customer_id} = $order_args{id};
    delete $order_args{password};
    delete $order_args{id};

    # Create order
    $order = $self->resultset('Orders')->create_order(\%order_args);

    # If our order is created then lets get our items
    if ($order->id) {
    # We found an our order id

        # Get the items from the cart    
        $cart_items = $self->resultset('CartItem')->get_items($cart->id);

        # Add the items in the cart to our order
        foreach my $row ( @$cart_items ) {
            $order_items{order_id}      = $order->id;
            $order_items{order_sku}     = $order->id.''.$row->sku;
            $order_items{sku}           = $row->sku;
            $order_items{quantity}      = $row->quantity;
            $order_items{price}         = $row->product->price;
            $order_items{name}          = $row->product->name;
            $order_items{weight}        = $row->product->weight;
            $order_items{weight_type}   = $row->product->weight_type;

            $self->resultset('OrdersItem')->add_item(\%order_items);
        }
    }
    
    return $order;

}

# process payment

# return customer authorization #

# email copy of order
 

1;
