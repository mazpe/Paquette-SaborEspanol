package MyOrder;

use Moose;
use MooseX::Types::Moose qw(Str Int ArrayRef);
use namespace::autoclean;
use Data::Dumper;
use WWW::Curl::Easy;
use LWP::UserAgent;

has 'user'          	=> ( is => 'ro', required => 1, weak_ref => 1 );
has 'schema'    	=> ( is => 'rw', required => 1, handles => [qw / resultset /] );
#has 'order_id'       	=> ( is => 'ro', isa => Int, default => 0 );

sub create_order {
    my ( $self, $customer_id ) = @_;
    my $order;
    my $order_args;

    # Create a hashref with our order arguments 
    $order_args = ( { 
        customer_id => $customer_id,
    } );

    # Create the Orders
    $order = $self->resultset('Orders')->create( $order_args );
    
    return $order;
}

sub get_order {
    my ( $self, $args ) = @_;
    my $order;

    # Check how we are going to find our order
    if ($args->{customer_id}) {
    # by cusomter id
        
        $order = $self->resultset('Orders')->get_order_by_customer_id(
            $args->{customer_id}
        );

    } elsif ($args->{order_id}) {

        $order = $self->resultset('Orders')->get_order_by_id(
            $args->{order_id}
        );

    } 

    return $order;
}

sub sum_items_in_order {
    my ( $self, $order_id ) = @_;
    my $total_amount;

    $total_amount
        = $self->resultset('OrdersItem')->sum_items($order_id);

    return $total_amount;
}

# Return list of items
sub get_items_in_order { 
    my  ( $self, $order_id ) = @_;

    my $items_in_order 
            = $self->resultset('OrdersItem')->get_items( $order_id );

    return $items_in_order ? $items_in_order : 0;
}

# Removes items from the shopping order
sub remove_items_from_order { 
    my ( $self, $args ) = @_;
    my $order_id         = get_order_id($self);
    my $order_sku        = $order_id.''.$args->{sku};

    # Remove particular item from order
    $self->resultset('OrdersItem')->remove_item($order_sku);

    return 1;
}

# Update items in the shopping order
sub update_items_in_order {
    my ( $self, $args ) = @_;
    my $order_id         = get_order_id($self);
    my $items           = $args->{items};

    # Loop through each sku
    foreach my $sku ( keys %$items ) {

        # Create our order_sku identifyer
        my $order_sku = $order_id.''.$sku;

        # Update item in the order
        my $item = $self->resultset('OrdersItem')->update_item( {
            quantity    => $items->{$sku},
            order_sku    => $order_sku,
        } );

    }

    return 1;
}

# Clear items in the shopping order
sub clear_order {
    my $self            = shift; 
    my $order_id         = get_order_id($self);

    # Clear our shopping order items
    $self->resultset('OrdersItem')->clear_items($order_id);

    return 1;
}

sub assign_order {
    my $self = shift;
    my $customer_id;
    my $order_by_cid;
    my $order_by_sid;
    my %order_args;

    if ( $self->user ) {
    # Logged in

        # Get our customer id
        $customer_id = $self->user->id;

        # Do we own a order? whats our order_id
        $order_by_cid
            = $self->resultset('Orders')->get_order_by_customer_id(
                $customer_id
              );

        # Do we have an anonymous order? what our anonymous order id
        $order_by_sid
            = $self->resultset('Orders')->get_order_by_sid(
                $self->session_id
              );

        # Check that we have 2 seperate orders
        if ($order_by_cid) {
        # Found customer order

            if ( $order_by_cid->id ne $order_by_sid->id ) {
            # Seperate orders

                # Update Orders Items to the customer_order
                $self->resultset('OrdersItem')->set_items_order_sku(
                    $order_by_cid->id,
                    $order_by_sid->id,
                );

                # Delete anonymous order
                $self->resultset('Orders')->delete($order_by_sid->id);

                # Update our customer order with our current session
                %order_args = (
                    id          => $order_by_cid->id,
                    session_id  => $self->session_id,
                );
                $self->resultset('Orders')->update(\%order_args);

            }

        } elsif ($order_by_sid) {
        # Found session order
            # Update our customer order with our current session
            %order_args = (
                id              => $order_by_sid->id,
                session_id      => $self->session_id,
                customer_id     => $customer_id,
            );

            $self->resultset('Orders')->update(\%order_args);

        }
        
        
    } else {
    # Not logged in


    }

}

sub set_shipping {
    my ( $self, $args ) = @_;
    my $order_id         = get_order_id($self);

    $args->{id} = $order_id;
    delete $args->{submit};
    
    # Set shipping
    $self->resultset('Orders')->set_shipping_info($args);
}

sub set_payment {
    my ( $self, $args ) = @_;
    my $order_id         = get_order_id($self);

    $args->{id} = $order_id;
    delete $args->{submit};

    # Set payment
    $self->resultset('Orders')->set_payment_info($args);
}

sub total_items_in_order {
    my $self            = shift;
    my $order_id         = get_order_id($self);


    my $total_items = $self->resultset('OrdersItem')->count_items($order_id);

    return $total_items;
}

sub destroy_order {
    my ( $self, $args ) = @_;

    # Destroy the order items
    $self->resultset('OrdersItem')->clear_items($args);
    
    # Destroy the order;
    $self->resultset('Orders')->delete($args);


}

sub process_cc {
    my ( $self, $args ) = @_;
    my $order;
    my $merchant_url;
    my $response_body;
    my %resp;
    my %form;
    my @test;
    my $ua;
    my $cc_exp;

    $order = $self->resultset('Orders')->get_order_by_id( $args->{order_id} );
    $cc_exp = sprintf("%02d", $order->credit_card_exp_month) . sprintf("%02d", $order->credit_card_exp_year);

    $merchant_url
        = 'https://secure.merchantonegateway.com/api/transact.php';

    %form = (
        type        => 'sale',
        username    => 'username',
        password    => 'password',
        ipaddress   => $ENV{'REMOTE_ADDR'},
        orderid     => $order->id,
        firstname   => $order->bill_first_name,
        lastname    => $order->bill_last_name,
        address1    => $order->bill_address1,
        city        => $order->bill_city,
        state       => $order->bill_state,
        zip         => $order->bill_zip_code,
        ccnumber    => $order->credit_card_number,
        ccexp       => $cc_exp,
        cvv         => $order->credit_card_cvv,
        amount      => $order->payment_amount,
    );

    $ua = LWP::UserAgent->new;
    $ua->timeout(10);
    $ua->env_proxy;

    $response_body = $ua->post($merchant_url, \%form );

    if ($response_body->is_success) {

        # $response_body: response=1&authcode=1234
        my @responses = split(/&/, $response_body->decoded_content);

        # $response: response=1
        foreach my $response (@responses) {
            my ( $key, $value ) = split(/=/, $response);
            $resp{$key} = $value;
        }

        if ( $resp{response} eq 1 ) {
            
            $self->resultset('Orders')->update_order( {
                order_id        => $resp{orderid},
                txn_id          => $resp{transactionid},
                txn_status      => $resp{response},
                txn_auth_code   => $resp{authcode},
                txn_avs         => $resp{avsresponse},
                txn_cvv         => $resp{cvvresponse},
                txn_code        => $resp{response_code},
                status          => 'PAID',
            } ); 

        } else {
            $self->resultset('Orders')->update_order( {
                order_id        => $resp{orderid},
                txn_id          => $resp{transactionid},
                txn_status      => $resp{response},
                txn_auth_code   => $resp{authcode},
                txn_avs         => $resp{avsresponse},
                txn_cvv         => $resp{cvvresponse},
                txn_code        => $resp{response_code},
                status          => 'PENDING PAYMENT',
            } );

        }

    } else {
        die $response_body->status_line;
    }
return \%resp;
}

1;
